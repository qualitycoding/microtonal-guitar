# READY — Verify and remediate microtonal-guitar issues

Profiles: `software`; `software.deploys=true`. Claims: 4 verified. Steps: 14. Gates: G-004, G-002, G-003. Risks: 0 Critical after controls, 6 High with mandatory controls, 1 Medium. Assumptions: 3 confirmed, 2 unverifiable with gates. Code review and adversarial review are implementation steps, not pre-executed results.

This plan is executable without further questions except at G-004, G-002, and G-003. Do not treat issue closure as proof. Commit and push after every green chunk and verify remote HEAD. Never swallow failures or close an issue without its traceability evidence.

## S-000 — Access, resume, and baseline

Verify authenticated identity and required repository permissions; load `domain:audio`, `tech:juce`, `tech:cpp`, and `tech:github-actions` lessons/knowledge; assert `git rev-parse origin/main` equals `504f5a93eeb081ac78776ef74b91142fe6307ef4` or restart planning from the newer baseline. Create/checkpoint all applicable artifacts. Inspect the existing #5 branch and recreate it from baseline if stale or unrelated.

Done when: access is proven, baseline is recorded, and `.checkpoints/state.json` contains the objective, branch, step pointer, variables, and artifact hashes.

## S-010 — Issue evidence

Create `ISSUE_EVIDENCE.md` with current source, PR/commit, reproduction or verification method, owning test, and closure link for every issue. Record PR #11 as the #10 wholesale-merge decision and mark the old generation branch superseded in evidence only; do not alter history.

Done when: all eight rows in TRACEABILITY have non-empty evidence and no row cites `9644510` as current code.

## S-020 — Test-first amendment

Preserve existing frozen files. Add boundary tests U-B1–U-B4 and integration test I-B1 for #5, plus I-A4 if audible refFreq measurement is absent. U-B cases cover zero, negative, NaN and infinite target frequency; invalid sample rate; delay below one sample; delay outside buffer; finite output; and recovery on a later valid note. Run against current code/stubs and commit red evidence. Re-freeze only after tests are valid; regenerate `tests/FROZEN.sha256`.

Done when: each new test fails for the intended reason before its fix, maps to an issue, and passes `sha256sum -c tests/FROZEN.sha256` after re-freeze.

## S-030 — Verify closed #4 without scope expansion

Review two-slot reader registration/publication and writer behavior; run the tuning-engine and concurrency tests. Check the complete audio path for locks, allocation, blocking, and torn reads. If acceptance fails, open a focused regression issue and use S-020 for its tests. Do not hide a writer-side fix inside #5.

Done when: test evidence and a written reader/writer review exist, or a linked regression issue blocks delivery.

## S-040 — Remediate #5

In `MicrotonalVoice::updateFrequency`, validate sample rate, target frequency, pitchbend-adjusted frequency, and computed delay. Require finite target > 0 and finite delay >= 1 sample and strictly inside the usable delay-buffer range. On invalid input set delay/excitation to a defined silent state. Apply the same finite/range defense in `renderNextBlock`. A later valid note must completely reinitialize the voice. Do not change valid-note tuning or timbre.

Done when: U-B1–U-B4 and I-B1 pass, every rendered sample is finite, valid-note regression tests pass, and no out-of-range access is possible.

## S-050 — Verify #6 and #8

Compare constructor/destructor listener sets and exercise repeated processor construction, parameter changes, and destruction under JUCE assertions. Confirm the root hand-written header is absent and clean builds use explicit JUCE module includes/generated build headers.

Done when: listener sets match, lifecycle test passes, no stale stub reference exists, and clean builds pass.

## S-060 — Complete #1 and #7 verification

Run the frozen manifest check, pure unit tests, tuning-engine tests, and all JUCE integration tests in mandatory CI. Use `ctest --output-on-failure --no-tests=error`; assert test executables/logs and VST3/Standalone artifacts exist and are non-empty. Verify I7 and I-A4 establish that refFreq changes actual output pitch by 432/440. Record O1–O19 and added O20–O22 in Ableton; O20 covers live tuning automation, O21 invalid/extreme parameter recovery, and O22 version/license evidence.

Done when: CI enumerates and passes every required suite, artifacts pass post-conditions, and operational results are attached. G-004 blocks otherwise.

## S-070 — Verify #9

Check SemVer documentation, `VERSION`/CMake/plugin-display consistency, Apache-2.0 SPDX headers on every first-party source file, third-party exclusions, and tag-triggered release workflow. Dry-run artifact and checksum production without tagging or publishing.

Done when: all checks pass and dry-run artifacts/checksums exist. Publication requires G-002.

## S-REVIEW — Traditional review

A fresh reviewer inspects the full diff for correctness, real-time safety, undefined behavior, test coverage, dependency drift, and unrelated changes. Write `reviews/CODE_REVIEW.md`; Critical/High findings block later steps. New tests follow S-020.

Done when: every finding links to a fix commit or explicit accepted risk and no Critical/High finding remains.

## S-ADVERSARIAL — Adversarial review

Attack zero/negative/NaN/infinite frequencies, extreme pitchbend and sample rates, delay boundaries, rapid parameter changes, invalid snapshots, repeated voice reuse, listener teardown, missing test executables, and malformed release metadata. Write `reviews/ADVERSARIAL_REVIEW.md`.

Done when: every confirmed break is fixed with a frozen regression test or explicitly accepted; Critical/High breaks block delivery.

## S-090 — Issue disposition

Update `ISSUE_EVIDENCE.md`. Close #5 only after its tests, CI, and reviews pass. For each closed issue, add a concise verification comment if evidence is missing; open a focused regression issue for any failed criterion. Do not reopen resolved work merely to match this plan.

Done when: every issue is either verified closed or has a current, linked remediation issue; no acceptance criterion is unaccounted for.

## S-100 — Release gate and final push

At G-004 present operational evidence; at G-002 present final commit, CI, reviews, version, artifacts, and checksums. Only with approval tag/publish. Push the remediation PR and verify remote/local HEAD. No release is required to close #5.

Done when: approved release evidence exists, or no-release is explicitly recorded and no external publication occurred.

## S-RETRO — Retrospective

Diff environment/commands against `DEVIATIONS.md`; record overlooked checks, recurring mistakes, and new reusable facts. Link every finding to a lesson or knowledge item and validate schemas/secret scan.

Done when: `RETROSPECTIVE.md` is complete, all findings have L-/K- links, and no secrets are present.

## S-KNOW — G-003

Write `GATE-G-003.md` listing lessons and knowledge. Push only after `push-to-proposed` or an explicit alternate target; honor `do-not-push`. Otherwise provide bundle fallback and stop.

Done when: approved stores contain indexed entries, `do-not-push` is recorded, or bundle paths are reported.
