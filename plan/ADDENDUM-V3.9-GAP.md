# Addendum v3.9 gap ledger

**Repository:** `qualitycoding/microtonal-guitar`
**Baseline:** `v3.6`  **Target:** `v3.9`
**Audited source commit:** `5a8b6e4e7ff3ac3cde052c66f4d013b146872186`
**Source branch tip at implementation:** `d603c8ea9ca7113507a7b70f699691698a6d2e67` (addendum publication commit on top of the audited commit; deviation D-901)

## Gap table (from ADDENDUM-PLAN-V3.9.md)

| Introduced | Missing capability | Amendment step |
|---|---|---|
| v3.7 | Separate Lessons and knowledge stores throughout intake, checkpoints and G-003 | `split` step below |
| v3.8 | Intake-derived versioned CI platform matrix with explicit coverage or waivers | `ci` step below |
| v3.9 | Priced three-tier mutation CI with one aggregator and a report-based score gate | `mutation` step below |

Every capability not listed above is treated as present in the baseline and is
re-verified by the final mechanical consistency check (`v39_addendum_verify.py
final`). Product scope, public interfaces, frozen expected behaviour,
performance targets and human gates are unchanged (D-900).
