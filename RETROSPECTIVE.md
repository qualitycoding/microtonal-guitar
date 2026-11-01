# Retrospective — Issue Remediation Run (gen-20261101T-issue-remediation)

Date: 2026-11-01/02. Scope: plan/issue-remediation/PLAN.md steps S-000…S-KNOW.

## What was done
- Verified issues #1–#10 with recorded evidence (ISSUE_EVIDENCE.md). #4, #6, #7, #8, #9 were already remediated on main via PRs #13, #15–#18; verification confirmed each fix.
- Remediated #5 (unguarded division in `MicrotonalVoice::updateFrequency`) test-first:
  - Red: U-B1..U-B4 boundary tests fail to compile on baseline 504f5a9; I-B1 segfaults on baseline (research/red-evidence/I-B1_RED.txt).
  - Green: new pure seam `FrequencyGuard.h::safeDelayLength()` (sample-rate, target-frequency, and delay-buffer-size validation; silent state on violation), silent-state contract in `renderNextBlock`, full voice re-init on next valid note.
  - Suites: unit 100%, tuning_engine 100%, boundary 4/4, integration harness 42 units incl. I-B1 (guard + recovery), I-B2 (lifecycle/listener symmetry, #6), I-A4 (autocorrelation pitch ratio 432/440).
- Repaired the stale frozen test manifest (r4→r5) after PRs #16–#18 changed includes without re-freeze; process miss filed as a regression issue.
- Reviews: reviews/CODE_REVIEW.md (LOW-1, LOW-2 accepted) and reviews/ADVERSARIAL_REVIEW.md (A1–A14) both complete, no Critical/High open.
- CI: unit-tests + integration-tests (ubuntu) + plugin matrix wired; integration post-condition fixed to locate the binary under JUCE `_artefacts`.

## Gate status
- G-004 (Ableton operational evidence): BLOCKED — no Windows/Ableton host available; O1–O22 recorded as not run. No operational closure claimed.
- G-002 (release): NOT exercised — no tag/release created; release job remains tag-gated (skipped in every run).
- G-003 (lessons/knowledge pushed to external stores): PENDING owner sign-off — see GATE-G-003.md.

## Lessons (this run)
- L-20261101T124000Z-sandbox-recycle-mid-build — sandbox recycles wipe /tmp mid-build; stage toolchains on the durable mount.
- L-20261101T130000Z-mount-build-cache-and-split-units — JUCE 9 split unity files and amalgamated C units; build on /tmp, cache objects on the mount (FUSE noexec + BFD assertion workaround).

## Knowledge items (this run)
- K-20261101T-guard-seam-pattern — pure validation seam (header-only, C++17, no JUCE) keeps red/green evidence host-independent.
- K-20261101T-frozen-manifest-refreeze — any PR touching frozen test files must re-freeze the manifest in the same PR; add a CI check.
