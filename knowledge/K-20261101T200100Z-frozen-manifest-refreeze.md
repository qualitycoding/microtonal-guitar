---
id: K-20261101T200100Z-frozen-manifest-refreeze
statement: "A frozen test manifest (tests/FROZEN.sha256) is only as fresh as its last freeze; any PR that modifies a file listed in the manifest must re-freeze it in the same PR, and CI should carry a manifest-check job (sha256sum -c) so drift fails the build instead of the next remediation run."
status: current
supersedes: []
tags: [subject:frozen-test-manifest, tech:github-actions, tech:cmake, phase:verification]
applies_to_version: "microtonal-guitar tests/FROZEN.sha256 at r5"
source: { url: "", doi: "", title: "tests/FROZEN.md, tests/FROZEN.sha256, branch fix/issue-5-zero-freq-guard", locator: "issue #19", accessed: "2026-11-02", tier: 3 }
confidence: verified
derived_from: []
discovered_by: implementer
run: gen-20261101T-issue-remediation
recorded_at: 2026-11-01T20:01:00Z
---
Found during S-050 of the 2026-11-01 issue-remediation run: the r4 manifest was stale because PRs #16-#18 changed frozen files without re-freezing; re-frozen at r5 (18/18 OK). Process miss tracked as issue #19. Pushed to qualitycoding/knowledge @ 3e8f494.
