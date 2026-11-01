---
id: L-20261031T200500Z-exposed-pat-short-lived
title: A PAT pasted into a chat can be auto-revoked within minutes — treat it as single-use
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: High
tags: [tech:github, tech:authentication, phase:release, kind:permissions, kind:security, applies:all]
recorded_by: implementer
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: 2026-10-31T20:05:00Z
---
## Trigger
A GitHub PAT supplied in chat authenticates successfully (API returns the owner login; one push
lands), then minutes later the identical token returns "Bad credentials" / "Invalid username or
token" for both API and git operations.
## What went wrong
Assumed the token would remain valid for the rest of the session; planned to push in two batches.
The second push failed auth — the token had been revoked (almost certainly automated secret-scan
revocation, given the paste into a chat and the minutes-scale timing). Work sat unpushed until
a fallback bundle was refreshed.
## Correction
Recorded the blocked push honestly, refreshed the bundle fallback, and requested a fresh token
from the owner. No secret was written to any file at any point.
## Prevention rule
Treat any token that has appeared in a chat/transcript as revoked-until-proven-otherwise: verify
with one cheap API call before relying on it, push everything in a single batch immediately after
receiving it, and never plan multi-push sequences around one exposed token.
## Detection check
`curl -H "Authorization: Bearer $TOKEN" https://api.github.com/user` returning "Bad credentials"
while the same token succeeded earlier in the run.
## Evidence
This run: API `qualitycoding | OK` at 19:57Z, push landed; `Bad credentials` at 20:04Z.
