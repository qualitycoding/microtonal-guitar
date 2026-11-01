# CODE_REVIEW.md — issue-remediation diff (S-REVIEW)

Reviewer: implementing agent, self-review pass per protocol S-REVIEW (no Critical/High findings may remain).
Scope: full diff `origin/main...fix/issue-5-zero-freq-guard` @ HEAD.
Lens: correctness, real-time safety, undefined behavior, test coverage, dependency drift, unrelated changes.

## Verdict

No Critical or High findings. Two Low findings, both explicitly accepted (linked below). Test
coverage for the change is test-first (red evidence in `research/red-evidence/` and
`tests/boundary/RED_EVIDENCE.txt`) and regression-frozen (r5, 18 manifest entries).

## What the diff does

- `FrequencyGuard.h`: new pure validation seam `safeDelayLength(sampleRate, targetFreq,
  delayBufferSize)`; returns a finite in-range delay length or 0.0 (defined silent state).
- `MicrotonalVoice.cpp`: `updateFrequency()` routes through the guard, clears the excitation on
  invalid state; `renderNextBlock()` re-validates before touching the delay line (defense in
  depth) and still completes a tail-off so the voice can clear.
- Tests: `tests/boundary/` (U-B1–U-B4), `tests/integration/` I-B1 (white-box invalid-input
  recovery), I-A4 (audible refFreq ratio 432/440), I-B2 (lifecycle, #6); tuning_engine guard
  cases moved to the 3-arg contract; manifest re-frozen (r5).
- CI: boundary suite in `unit-tests`; new mandatory `integration-tests` job; plugin matrix now
  depends on it.

## Checks performed

| Check | Result |
|---|---|
| Real-time safety (audio thread) | Guard is plain arithmetic (`isfinite`, compares) — no locks, no allocation, no blocking. `renderNextBlock`'s silent path touches only `tailOff`. PASS |
| Undefined behavior | Baseline's `(int)(-inf)` + negative modulo indexing is eliminated: every render path is gated on a finite, in-range delay. PASS |
| Valid-note behavior unchanged | Guard returns `sampleRate / targetFreq` unchanged for all valid inputs (asserted numerically in U-B1–U-B4 and tuning_engine suite); frozen I1–I16 regression green. PASS |
| Dependency drift | JUCE pin unchanged (`MICROGUITAR_JUCE_TAG=9.0.4`); no new third-party deps in the product (doctest was already vendored). PASS |
| Unrelated changes | None in product code; test/CI/docs changes only. The accidentally-committed cmake cache was removed in 5ff82ec with `build-*/` ignored. PASS |
| Frozen-file integrity | `sha256sum -c tests/FROZEN.sha256` — 18/18 OK. PASS |

## Findings

### LOW-1 (accepted): pitch-wheel routing never reaches held notes on removed strings
`processBlock`'s pitch-wheel fan-out iterates `s < numStrings` (`currentNumStrings`). If the
string count shrinks while a note rings on a higher string, that voice never receives bends, so
it keeps its last delay length. With the #5 guards this is benign (no crash/NaN; the worst case
is a held note that cannot be bent until re-articulated). Fixing the routing fan-out is a
behavior change outside #5's scope (D-003); recorded here and in `ADVERSARIAL_REVIEW.md` as an
accepted risk. Suggested as backlog, not filed as a blocking issue.

### LOW-2 (accepted, pre-existing): `delayBuffer.setSize` allocates on the audio thread
`noteStarted()` sizes the delay buffer with `currentSampleRate + 1` samples. Pre-existing
(baseline and prior releases), allocation happens on the first note, not per block. Out of #5
scope per D-003/D-007; accepted with the note that a future enhancement could pre-size in
`prepareToPlay`.

## Conclusion

All Critical/High checks pass; LOW-1/LOW-2 accepted. S-REVIEW complete.
