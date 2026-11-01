# DEVIATIONS.md

Every substitution made during this run (Rule 14: substitutions are deviations, logged when made).

| # | Substitution | Reason | Logged at |
|---|---|---|---|
| D-1 | CMake/JUCE build not executed; replaced by a g++-only spike of the JUCE-free `TuningEngine` | cmake absent from sandbox; JUCE path hardcoded to D:/JUCE; build exceeds container lifetime. Dimension recorded as environment-blocked with CI named as the honest completion path (1.5 item 6) | 2026-10-31T19:48Z |
| D-2 | Rule 6 push cannot land (no GitHub credentials in the sandbox); executed the pre-decided fallback: `git bundle create` + path report | 0.1 found no push credential; per Rule 6/Phase 5 item 3 a failed push falls back to a bundle rather than a silent skip | 2026-10-31T19:53Z |
| D-3 | Lesson L-20261031T195000Z: compiled binaries staged to `/tmp` for execution | output mount forbids exec despite chmod success | 2026-10-31T19:50Z |

No version pins, flags, or timeouts were substituted. No frozen tests exist to be affected
(full freeze is out of addendum scope; see REVIEW.md G-1).
| D-4 | D-ADD-004 fallback superseded: the run owner supplied a GitHub credential after the bundle fallback; the branch was pushed to origin and verified with `ls-remote` | 0.1 found no credential in-sandbox; owner provided one on request | 2026-10-31T19:57Z |
| D-5 | Merged the prior unmerged generation branch r4 into the addendum branch at owner request | Owner asked to bring anything missing from the prior branch over; merge --no-ff preserves both histories | 2026-10-31T20:00Z |
| D-6 | Push of the merge blocked: the owner-supplied PAT was revoked between 19:57Z and 20:04Z | Token paste is presumed auto-revoked by secret scanning; treated as single-use from now on (lesson L-...05Z) | 2026-10-31T20:05Z |
| D-6-resolved | Fresh token supplied by owner; merge push landed and verified | Supersedes the blocked state of D-6 | 2026-10-31T20:07Z |
| D-7 | Toolchain staged from GitHub release via ghfast.top proxy (cmake 4.4.5) and JUCE 9.0.4 cloned via the same proxy; both cached on the durable mount for restaging | Direct github.com large-file throughput was ~10KB/s in this sandbox; proxy mirrors are the recorded alternative channel (0.1 reachability rule). CI uses github.com natively — no proxy in the workflow | 2026-10-31T20:05Z |
| D-8 | JUCE integration harness built with a resumable direct-g++ script (`research/scripts/build_integration.py`) and object cache on the durable mount, instead of a monolithic CMake build in one sandbox | sandbox recycles within ~15 min and wipes /tmp; the full JUCE 9 build (42 units incl. split unity files `juce_gui_basics_2..5`, `juce_audio_processors_headless`, plutovg/harfbuzz/sheenbidi amalgamations) cannot finish in one lifetime; CMake path remains the CI-canonical one | 2026-11-01T13:30Z |
| D-9 | Full VST3/Standalone plugin build remains environment-blocked locally (no root for ALSA/X11 dev packages; exceeds container lifetime) | identical job exists in CI (`plugin` matrix); local verification = integration harness rendering real audio + all pure suites | 2026-11-01T13:30Z |
| D-10 | I-B1 triggers the invalid state via a white-box `updateFrequency()` call (`#define private public` confined to first-party headers) | the product's pitch-bend fan-out only addresses strings < currentNumStrings, so MIDI alone cannot deliver the invalid state (recorded as LOW-1 in reviews/CODE_REVIEW.md) | 2026-11-01T13:45Z |
| D-11 | Ableton operational tests O1–O22 not executed (no Windows/Ableton host) | honest blocked status per Rule 14; gated at G-004; no operational closure claimed | 2026-11-01T14:00Z |
| D-12 | CI runs for commit 61329cae were cancelled on 2026-11-01 ~19:49–19:56Z when the owner reset `gen-20261101T-issue-remediation` to the plan commit; all work is preserved on `fix/issue-5-zero-freq-guard` (PR #14) | external branch reset by repo owner; no force-push performed over the reset state; full matrix re-triggered via empty commit 2fc7a3b | 2026-11-02T00:00Z |
| D-13 | `plugin` matrix OS `macos-14` retired by GitHub: every `macos-14` job cancelled in 0–2 s with no runner ever assigned (all runs from 2026-11-01 14:36Z onward); diagnosed via scratch-branch probe on `macos-15`, which provisions and runs normally. Matrix moved to `macos-15` | environment change outside repo; probe branch `ci-probe-macos15` created and deleted after diagnosis | 2026-11-01T20:30Z |
