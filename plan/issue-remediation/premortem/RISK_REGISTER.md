# Risk register

| ID | Severity | Risk | Mitigation |
|---|---|---|---|
| R-001 | High | Stale research reimplements merged fixes | Assert baseline and current issue state in S-000; verify before changing code |
| R-002 | Critical | A check reports success without executing tests | `ctest --no-tests=error`, enumerated logs, executable/result/artifact post-conditions |
| R-003 | Critical | Frozen tests are weakened | Manifest gate and formal amendment/red-verification procedure |
| R-004 | High | Invalid delay handling permanently silences a voice | Defined silent state plus next-valid-note recovery tests |
| R-005 | High | A finite but out-of-range delay indexes outside the buffer | Require finite delay >= 1 and strictly inside usable buffer range; defensive render check |
| R-006 | High | Closed #4 RCU has an unexamined writer-side real-time defect | S-030 reviews writer and reader paths; any failure gets its own regression issue, not a hidden #5 expansion |
| R-007 | High | Manual operational coverage is assumed | G-004 blocks closure/release without recorded results |
| R-008 | High | Public release is premature or irreversible | G-002, dry-run artifacts/checksums, documented withdrawal and corrective PATCH |
| R-009 | Medium | Existing #5 branch contains stale or unrelated changes | Inspect its base/diff; recreate from baseline when necessary and log the deviation |
