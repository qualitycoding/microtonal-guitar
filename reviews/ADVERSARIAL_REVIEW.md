# ADVERSARIAL_REVIEW.md — issue remediation (S-ADVERSARIAL)

Attack surface: zero/negative/NaN/infinite frequencies, extreme pitchbend and sample rates,
delay boundaries, rapid parameter changes, invalid snapshots, repeated voice reuse, listener
teardown, missing test executables, malformed release metadata.

| # | Attack | Result | Evidence |
|---|---|---|---|
| A1 | targetFreq = 0 (engine rejects out-of-range string index) | Silent state; no inf/NaN; recovery on valid input | U-B1, I-B1 (red on baseline: segfault) |
| A2 | targetFreq negative / NaN / ±inf | Silent state | U-B2 |
| A3 | sample rate 0 / negative / NaN / inf | Silent state | U-B3 |
| A4 | computed delay < 1 sample (freq > sample rate) | Silent state | U-B4 |
| A5 | computed delay >= delay-buffer size (freq below ~1 Hz at 48 kHz) | Silent state | U-B4 |
| A6 | delay of exactly 1 sample (freq == sample rate) | Admissible per plan S-040 (>= 1 sample); render math valid | U-B4 (boundary pinned) |
| A7 | extreme pitchbend (±inf/NaN semitones bend the target frequency to non-finite) | Guard rejects non-finite bend-adjusted frequency (same seam as A1–A2) | U-B2 + guard code path |
| A8 | extreme sample rates (1 Hz … 192 kHz, non-finite) | Buffer sized `sampleRate + 1`; guard keeps delay strictly inside; low rates fall to silent state | U-B3/U-B4, render guard |
| A9 | rapid parameter changes (string count, refFreq, instrument, tuning mode, edoSteps) | 200 construct/change/destroy cycles, no leak/assert; engine 40k-write × 3-reader stress | I-B2, tuning_engine suite |
| A10 | invalid/legacy snapshots | Legacy `tuningType` migration round-trips | frozen I4, I16 (harness run, 0 failures) |
| A11 | repeated voice reuse after invalid state | I-B1 recovery: valid reconfiguration fully reinitialises; finite, audible output | I-B1 |
| A12 | listener teardown (#6) | add/remove sets identical; 200 destructor cycles clean | I-B2, `ISSUE_EVIDENCE.md` S-050 note |
| A13 | missing/empty test executables | CI post-conditions on every suite (`test -x && test -s`, VST3/Standalone dir contents) | build.yml |
| A14 | malformed release metadata (#9) | `VERSION` format enforced by CMake `FATAL_ERROR`; tag pattern `v*` gates release job; no tag pushed | S-070 checks |

## Confirmed break, fixed

- Baseline crash/NaN on invalid frequency: **fixed** (guards + defined silent state), with
  frozen red→green evidence (research/red-evidence/, tests/boundary/RED_EVIDENCE.txt).

## Confirmed break, explicitly accepted (Low, non-blocking)

- Pitch-wheel fan-out drops bends for held notes on strings >= currentNumStrings (LOW-1 in
  `CODE_REVIEW.md`). Benign under the new guards; behavior change deferred (D-003 scope rule).

## Notes

- The white-box `#define private public` in `VoiceGuardAndPitchTests.cpp` is confined to
  first-party headers included after all JUCE headers; it exists because the product's MIDI
  routing cannot deliver the invalid state (which is itself LOW-1).
- O20–O22 (Ableton operational) are environment-blocked here and gate at G-004; no operational
  closure is claimed.

No Critical/High breaks remain open. S-ADVERSARIAL complete.
