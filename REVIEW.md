# Repository review: microtonal-guitar vs planning-protocol v3.6

Reviewed: commit `9644510` (`main`), 5 commits, 9 source files, ~1050 LOC (JUCE C++ VST3/Standalone).
Protocol: `qualitycoding/skills` `protocols/planning-protocol.md` **v3.6** (recorded 2026-10-30),
change record `protocols/planning-protocol-review.md` (Round 6 complete).
Profile selection per Rule 0: **`software`** (producing code intended to run/ship). No math,
computational, or publication deliverables. Environment class per Phase 0.1: **ephemeral**
(Kimi cloud; evidence in `.checkpoints/state.json`), so every `[ephemeral]`-tagged section applies.

## Headline finding

The repository has **never been through the protocol**. None of the Phase 1–5 artifacts exist:
no `plan/`, no `HANDOFF.md`, no `.checkpoints/`, no `tests/` (frozen or otherwise), no CI, no
`reviews/`, no `lessons/` or `knowledge/` directories, no `EXECUTION_LOG.md`/`DEVIATIONS.md`.
Every applicable protocol artifact is a gap. This review therefore records the load-bearing gaps
and the code-level findings; the addendum plan (`ADDENDUM-PLAN.md`) covers only the steps **added
in the v3.6 protocol update** (the ephemeral-environment protections and the universal push
cadence), which are implemented on this branch.

## Gaps vs the protocol (severity per Global Rule 4)

| # | Severity | Gap | Protocol ref |
|---|---|---|---|
| G-1 | Critical | No tests of any kind: no unit tests for `TuningEngine`, no integration test for string allocation, no red-verified frozen suite. "A verification that reported success without having run" is impossible here because nothing runs at all. | 2B, 2E, Rule 14 |
| G-2 | Critical | No CI. Nothing catches regressions; there is no honest-verification surface. | Rule 14, 1.5 item 6 |
| G-3 | High | `CMakeLists.txt` hardcodes `add_subdirectory("D:/JUCE" JUCE)` — a Windows-only absolute path. The project cannot configure on any other machine; setup commands are unverifiable as shipped. | 1.4, 1.3.6 |
| G-4 | High | `currentNumStrings` is never updated: the `stringCount` parameter is pushed into `TuningEngine`'s state, but the processor's `std::atomic<int> currentNumStrings` stays at its initial value 6 forever. `processBlock` allocates strings using this atomic, and `getNumStrings()` reports it to the UI — the headline "Dynamic String Count" feature (4–12 strings) silently does nothing outside the default 6. | 2B (tests would have caught this) |
| G-5 | High | Real-time safety: `MicrotonalVoice::updateFrequency()` calls `TuningEngine::getFrequency()`, which takes a `std::mutex`, from the audio thread (inside `processBlock`). Locking a mutex in the audio callback risks priority inversion and underruns; JUCE guidance and this protocol's `[software]` scale/performance lens treat this as a defect. | 3.8 S-REVIEW lens |
| G-6 | Medium | `MicrotonalVoice::updateFrequency()` divides by `targetFreq` with no guard: `getFrequency` returns 0.0 for an out-of-range string index, producing `inf`/`NaN` delay length (the existing `<= 0.0` guard does not catch `inf`). Edge-case failure path is unhandled. | 2B test categories |
| G-7 | Medium | Constructor adds parameter listeners for `stringCount`/`tuningType`/`refFreq` but the destructor removes a `musicalKey` listener that was never added — evidence of unreviewed iterative patching (the "ADDED"/"FIXED" comment fossils). Low risk functionally, High risk as a process signal: no code review has ever run on this diff. | 3.8 S-REVIEW |
| G-8 | Medium | `GuitarState.referenceFrequency` is set from the `refFreq` parameter but never used in frequency computation; the A4 reference slider (README feature) is inert. | 2B |
| G-9 | Low | `JuceHeader.h` in repo root duplicates JUCE's generated header pattern but is hand-written and unused by the CMake target; dead file. | 3.8 S-REVIEW |
| G-10 | Low | No versioning scheme, no release channel, no license headers in sources, no CONTRIBUTING/issue templates. | 0.3.2 |

## What the repo already does well

* Thread-safety intent is mostly right: `activeFrets` and voice state use atomics for UI reads;
  `TuningEngine` snapshots are taken under a lock and returned by value.
* The `openStringMidiNote` single-source-of-truth helper is a good design decision (one place
  defines the string/pitch mapping used by voice, processor, and UI).
* Karplus-Strong fractional-delay synthesis and the string-allocation rule ("highest open string
  at or below the note, lowest fret") are implemented cleanly and match the README's claims.

## Applicability of the v3.6 additions to this repo

* **Phase 0.1 environment-class probe + 1.5 `[ephemeral]`** — applicable now (this run executes
  in an ephemeral Kimi-cloud sandbox). Implemented: see `.checkpoints/state.json` and
  `plan/ENVIRONMENT.md`.
* **Rule 6 push cadence** — applicable on all platforms. Implemented on this branch: commits are
  made per chunk, push attempted, `ls-remote` verification recorded, and (no credentials being
  available in this sandbox) the bundle fallback executed. See `EXECUTION_LOG.md` and
  `DEVIATIONS.md`.
* **3.4 resiliency hook** — restated in the addendum plan steps.
* **3.5 HANDOFF restatement** — included in `ADDENDUM-PLAN.md` (Rules 6/11/12/13 restated there).

## Note on scope

A full protocol run (research rounds R0–R6, test freeze, pre-mortem Phases 4–5, code review
steps S-REVIEW/S-ADVERSARIAL) is **out of scope** for this addendum: the user asked for an
addendum plan of the *newly added* protocol steps, not a full plan. The full-protocol gaps
above (G-1…G-10) are recorded so a future full run picks them up as risks and frozen tests.
