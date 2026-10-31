---
id: L-20261031T195000Z-fuse-mount-exec-bits
title: Output mount accepts chmod but forbids execution — stage binaries to local disk
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: Medium
tags: [tech:sandbox, phase:implementation, kind:environment, applies:all]
recorded_by: implementer
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: 2026-10-31T19:50:00Z
---
## Trigger
A freshly compiled binary on the persistent output mount fails with `Permission denied` even
after `chmod +x` succeeds without error (fuse-style mount that silently drops exec bits).
## What went wrong
Ran the compiled spike via `./research/spikes/tuning_engine_spike` → Permission denied; retried
after `chmod +x` → still Permission denied (a correction of approach, not a transient retry).
## Correction
Copied the binary to `/tmp`, chmodded there, ran from local disk — success.
## Prevention rule
On mounts that forbid exec, never invoke `./binary` from the mount; copy to a local disk first
(or run scripts via an interpreter), and record the staged copy in the execution log.
## Detection check
`./some_binary` on the mount returns Permission denied while `/tmp/some_binary` runs.
## Evidence
`research/spikes/tuning_engine_spike.output.txt` (`SPIKE RESULT: ALL PASS` after staging to /tmp).
