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
