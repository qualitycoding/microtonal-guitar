# ISSUE_EVIDENCE.md

Evidence register for the issue-remediation plan (`plan/issue-remediation/PLAN.md`).
Baseline for all verification: `main` @ `504f5a93eeb081ac78776ef74b91142fe6307ef4`.
Rule: closure is not proof — each row records the verification performed under this plan.
Nothing below cites `9644510` as current code; that commit is the obsolete pre-merge baseline
(D-001) and appears only in the legacy-state test fixture (which intentionally reproduces its
APVTS shape).

| Issue | Current source | PR / commit | Verification method | Owning test(s) | Closure evidence |
|---|---|---|---|---|---|
| #1 tests | Closed — suite on `main` (unit U1–U15, integration I1–I16, operational O1–O19, manifest) | PR #11 (merge of addendum+r4 lines) | S-020 manifest check (`sha256sum -c tests/FROZEN.sha256`); S-060 enumerated runs + CI post-conditions | `tests/unit/*`, `tests/integration/*`, `tests/tuning_engine/*`, CI `unit-tests` job | CI run + frozen-manifest check (S-060); results below |
| #4 audio-thread lock | Closed — two-slot RCU in `TuningEngine` | PR #13 (`fix/audio-thread-lockfree-tuning`) | S-030 reader/writer review + `tests/tuning_engine` suite (behaviour + 40k-write 3-reader stress) | `tests/tuning_engine/test_tuning_engine.cpp` | S-030 review note in this file + green suite log |
| #5 zero/non-finite delay | **Open** — unguarded division in `MicrotonalVoice::updateFrequency` on baseline | PR #14 (this branch, open) | S-020 red U-B1–U-B4 / I-B1 → S-040 guards → green | `tests/boundary/test_voice_frequency_guards.cpp` (U-B1–U-B4), integration I-B1 | Red+green logs committed; see S-040 evidence |
| #6 listener mismatch | Closed — constructor/destructor sets match | PR #15 | S-050 diff of add/remove sets + repeated construct/change/destroy lifecycle | inspection + lifecycle exercise (S-050) | S-050 note below |
| #7 refFreq inert | Closed — refFreq drives `openStringFrequencies` | PR #16 | S-060 I7 (engine ratio 432/440) + new I-A4 (rendered output pitch ratio 432/440) | `tests/integration/TuningParameterTests.cpp` I7; new I-A4 | S-060 log; I-A4 red→green |
| #8 root JuceHeader | Closed — hand-written header deleted | PR #17 | S-050 repo-tree check + clean 3-platform build via CI; sources include explicit module headers | CI `plugin` matrix (3 OS) | S-050 note; CI run |
| #9 version/release/headers | Closed — VERSION file, SemVer check, SPDX headers, tag release workflow | PR #18 | S-070 header scan, VERSION/CMake consistency, release dry-run artifacts+checksums (no tag/publish) | `reviews/` dry-run artifacts | S-070 log; G-002 blocks publication |
| #10 stale generation branch | Closed — owner decision: wholesale merge via PR #11; old branch superseded (evidence only, history untouched) | PR #11 / PR #12 | S-010 merge evidence: r4 content present on `main` (tests, NoteNaming, Instruments, EdoControl); branch still exists remotely, marked superseded | n/a | this row + merge commit `a8aad92` in `EXECUTION_LOG.md` |

## S-030 — #4 verification note

Reader path (`beginRead`/`endRead`/`getFrequency`/`getSnapshot`): lock-free registration on the
published slot; readers hold a plain reference for a few reads; no allocation, no blocking.
Writer path (`setGuitarState`): takes `writerMutex` (never held by the audio thread), spins only
on `readers` of the slot being overwritten (nanosecond-scale), publishes with release-store.
Full audio-path inspection (S-030): the only engine calls on the audio thread are the readers;
`processBlock` allocates nothing per block except the MIDI pass-through; UI reads go through
atomics (`currentNumStrings`, `activeFrets`) or the lock-free snapshot. No torn reads: slot
index + readers counter are atomic; state copy happens under registration. No writer-side fix
hidden in #5 (D-007) — the writer mutex is documented as UI-side only.

## S-050 — #6 / #8 verification notes

- #6: constructor adds `stringCount`, `refFreq`, `instrument`, `tuningMode`, `edoSteps`;
  destructor removes exactly the same five. The spurious `musicalKey` removal is gone (PR #15).
  Lifecycle exercise: 200 rounds of construct → change each parameter → destroy under JUCE
  assertions enabled; no leak/assert output (see `reviews/` logs).
- #8: no `JuceHeader.h` in repo root; sources include explicit module headers
  (e.g. `juce_audio_basics/juce_audio_basics.h`) and JUCE's generated build headers via CMake.

## S-060 / S-070 / S-090 updates

Appended during execution — see `EXECUTION_LOG.md` and the sections below.
