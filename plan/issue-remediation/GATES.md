# Human gates

## G-004 — Operational evidence

Before operational issue closure or release, present O1–O22 results, host and Live versions, build commit, and every failure. Incomplete or failed results block release. Do not infer a pass from CI.

## G-002 — External release

Before any tag or GitHub Release, present the commit, version, CI run, reviews, operational evidence, artifacts, and checksums. Publish only after explicit sign-off. Rollback is to withdraw the release/tag and publish a corrective PATCH; never reuse a version.

## G-003 — Lessons and knowledge stores

After S-RETRO, list lesson and knowledge IDs and request `push-to-proposed`, alternate targets, or `do-not-push`. Do not create or update external stores without sign-off.
