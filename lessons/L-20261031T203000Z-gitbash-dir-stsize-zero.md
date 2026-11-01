---
id: L-20261031T203000Z-gitbash-dir-stsize-zero
title: test -s on a directory is false on Git Bash (st_size 0) — check contents instead
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: Medium
tags: [tech:github-actions, tech:windows, phase:verification, kind:verification-gap, applies:all]
recorded_by: implementer
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: 2026-10-31T20:30:00Z
---
## Trigger
A CI post-condition uses `test -s <path>` to assert an artifact is non-empty; the artifact is a
bundle directory (.vst3 / .app). Job passes on ubuntu/macos runners but fails on windows-latest
with "missing or empty" even though the bundle is fully populated.
## What went wrong
On Git Bash/MSYS, directories report st_size 0, so `test -s dir` is false; on GNU/Linux and
macOS it is true. The check was platform-dependent and falsely red on Windows.
## Correction
Replaced the check with a helper: if the path is a directory, assert it contains at least one
file (`find "$1" -type f -print -quit` non-empty); else `test -s` as before. Windows CI then
went green with the same artifacts.
## Prevention rule
Never `test -s` a path that may be a directory; branch on `-d` and verify directory contents
(file count/size) instead.
## Detection check
Grep workflows for `test -s` applied to variables that can hold .vst3/.app paths.
## Evidence
Run 41303854774 job 124840189803: build succeeded, post-condition failed; fixed workflow re-run green.
