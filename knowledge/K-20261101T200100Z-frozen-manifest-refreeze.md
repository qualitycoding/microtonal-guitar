# K-20261101T200100Z-frozen-manifest-refreeze

- **applies:** all
- **domain:** domain:audio
- **tech:** tech-github-actions, tech-cmake

A frozen test manifest (`tests/FROZEN.sha256`) is only as fresh as its last
freeze. If a PR modifies files listed in the manifest without re-freezing in the
same PR, the manifest silently drifts and a later re-freeze step fails on
main-line content that changed under it.

**Rule:** any PR that touches a file listed in the frozen manifest must re-freeze
the manifest within the same PR, and CI should carry a manifest-check job
(`sha256sum -c tests/FROZEN.sha256`) so drift fails the build instead of the next
remediation run.

**Source:** microtonal-guitar issue-remediation run 2026-11-01 (S-050): r4
manifest was stale because PRs #16–#18 changed frozen files without re-freezing;
re-frozen at r5. Regression tracked as issue #19.
