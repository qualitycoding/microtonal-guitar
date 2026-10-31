---
id: L-20261031T195400Z-check-remote-branches
title: Enumerate all remote branches before claiming a repo "has no plan artifacts"
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: Medium
tags: [tech:git, tech:github, phase:research, kind:process, applies:all]
recorded_by: implementer
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: 2026-10-31T19:54:00Z
---
## Trigger
Writing a repo review from the default branch alone; `git clone` checks out `main` and prior
protocol runs live on unmerged `gen-*` branches.
## What went wrong
REVIEW.md initially claimed the repository "has never been through the protocol" and had no
tests/plan — true only of `main`. `git ls-remote` during the Rule 6 verification step revealed
`gen-20260919T224942Z-fret-note-names-r4` with a full older-protocol plan and frozen suite. The
review text had to be corrected and re-committed.
## Correction
Fetched and inspected the prior generation branch; rewrote the headline finding, qualified the
test/CI gaps as "`main`-only", and noted the merge-or-rescope decision for the human.
## Prevention rule
Before asserting absence of plan artifacts, run `git ls-remote --heads origin` and inspect every
`gen-*`/`impl-*` branch's file listing; state the branch each claim applies to.
## Detection check
Review contains the sentence "checked all remote branches: <list>" or equivalent `ls-remote` evidence.
## Evidence
Push-verification step of this run (`git ls-remote origin` output showing the r4 branch); corrected REVIEW.md commit.
