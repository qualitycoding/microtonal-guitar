# Addendum plan — steps added to the planning protocol in v3.6

Base plan: none on `main`; note that a **prior, unmerged generation branch from an earlier
protocol version exists remotely** (`gen-20260919T224942Z-fret-note-names-r4`, executed through
S1–S7) — see `REVIEW.md` headline. This addendum is based on `main` and does not modify that
branch; if that plan is ever resumed, it must be re-planned under v3.6 and this addendum's steps
folded in.
This addendum implements **only the steps introduced by the v3.5 → v3.6 protocol update**
(Round 6 in `planning-protocol-review.md`):

1. Phase 0.1 **execution-environment class probe** (`stable` vs `ephemeral`).
2. Phase 1.5 **Ephemeral-Environment Protections** (`[ephemeral]`, items 1–6).
3. Global Rule 6 **push cadence** — commit-and-push after every successful chunk, with
   `ls-remote` verification, backoff retries, and mismatch reporting, on **all** platforms.
4. The **3.4 resiliency hook** (`[ephemeral]` restatement) and **3.5 HANDOFF restatement**
   (Rules 6, 11, 12, 13 verbatim) that accompany the above.

Scope rule from the change record: recycling protections apply **only** on environments observed
or classified as recycling; push cadence is **active on all platforms**. Earlier-round additions
(checkpointing 1.2, assumption audit 4.7, review steps 3.8, lessons/knowledge 11–13) are not
re-planned here, but the checkpoint schema (1.2.2) and the lessons/knowledge machinery are used
where this addendum's steps require them.

Profiles: `software`. Environment class: `ephemeral` (Kimi cloud sandbox; probe evidence below
and in `.checkpoints/state.json`). Status: READY (steps below are the implementation record).

## Steps (3.1 template)

### S-ADD-001 Environment-class probe (Phase 0.1)
- Tier: Haiku
- Profile: All
- Depends on: none
- Inputs: sandbox shell
- Actions: (1) Record tool versions (`git --version`, `g++ --version`, `cmake --version`).
  (2) Write a marker to `/tmp` and read it back in a later tool call; record whether it survived.
  (3) Record whether `/mnt/agents` (persistent mount) survives across calls. (4) Record any
  deadline-kill observed during the run. (5) Classify per the observed table (Kimi cloud =
  ephemeral) and record class + evidence in `.checkpoints/state.json` and `plan/ENVIRONMENT.md`.
- Outputs: `.checkpoints/state.json`, `plan/ENVIRONMENT.md`
- Evidence produced: none
- Done when: state.json contains `environment.class = "ephemeral"` with probing evidence listed.
- On failure: probe is read-only; cannot fail. If evidence is ambiguous, classify unknown as
  ephemeral.
- Gate: none
- Relevant decisions/claims: D-ADD-001 (classification from observed table, not per-run guesswork)
- Lessons applied: none
- Exclusive resources: none

### S-ADD-002 Durable-surface discipline (1.5 item 1)
- Tier: Haiku
- Profile: All (this run: [ephemeral])
- Depends on: S-ADD-001
- Inputs: `/mnt/agents` persistent mount; repo working copy
- Actions: Keep all working files under the persistent mount; treat `/tmp`, env vars, installed
  packages, and running processes as ephemeral; copy anything a later turn needs into the mount
  within the same action that produces it.
- Outputs: repo working copy at `/mnt/agents/output/microtonal-guitar`
- Done when: every artifact produced by this addendum lives under the mount.
- On failure: if a write lands outside the mount, move it and record in `DEVIATIONS.md`.
- Gate: none
- Relevant decisions/claims: D-ADD-002
- Exclusive resources: none

### S-ADD-003 Turn-start restaging with exec-bit restoration (1.5 item 2)
- Tier: Haiku
- Profile: All (this run: [ephemeral])
- Depends on: S-ADD-002
- Actions: At the start of every turn, re-stage any toolchain/runtime pieces from the mount into
  fast local disk; restore executable bits after every copy (`chmod -R +x`, or run scripts via an
  interpreter where the mount forbids exec); verify staged binaries run before depending on them.
- Done when: each turn that invokes tools begins with the restage check recorded in
  `EXECUTION_LOG.md`.
- On failure: re-fetch/re-stage from source; record in `DEVIATIONS.md` if the source differs.
- Gate: none
- Relevant decisions/claims: D-ADD-002
- Exclusive resources: none

### S-ADD-004 Checkpointed, resumable work and ordering invariants (1.5 items 3–4)
- Tier: Haiku
- Profile: All (this run: [ephemeral])
- Depends on: S-ADD-002
- Actions: Long work executes as a resumable loop with periodic sync to the mount (this addendum
  is short enough to checkpoint per step; `.checkpoints/state.json` is written and committed after
  every step — atomic write-then-rename, snapshots under `.checkpoints/snapshots/`). Ordering
  invariants: pass dependency overrides as `-D` configure arguments, never env vars; back-date or
  precompile generated headers only after the final configure; export `compile_commands.json` at
  configure time; cache downloaded archives on the mount.
- Outputs: `.checkpoints/state.json`, `.checkpoints/snapshots/checkpoint-*-seq-*.json`
- Done when: state.json sequence increments per step; `in_flight_operation` is null at rest.
- On failure: re-run only the interrupted step; reconcile side effects idempotently (1.2.3).
- Gate: none
- Relevant decisions/claims: D-ADD-002
- Exclusive resources: none

### S-ADD-005 Right-sized work and evidence honesty (1.5 items 5–6)
- Tier: Sonnet
- Profile: All (this run: [ephemeral])
- Depends on: S-ADD-001
- Actions: (1) Keep every step within container-lifetime bounds (minutes). (2) A verification
  dimension that cannot complete locally because of recycling is documented **blocked
  (environment limitation)**, with the identical job named for CI, and is **never reported as
  green**. Concretely: `cmake` is absent in this sandbox and a full JUCE VST3 build exceeds one
  container lifetime, so the full plugin build is recorded as environment-blocked, with the CI
  job named; the locally feasible verification (standalone g++ spike of `TuningEngine`) is run
  instead.
- Outputs: `plan/ENVIRONMENT.md` (blocked-dimension register), `research/spikes/`
- Done when: each verification dimension is labeled pass / blocked-with-CI-name / not-run —
  none silently green.
- On failure: n/a
- Gate: none
- Relevant decisions/claims: D-ADD-003
- Exclusive resources: none

### S-ADD-006 Push cadence after every chunk (Rule 6)
- Tier: Haiku
- Profile: All — active on every platform, stable and ephemeral
- Depends on: every step above (runs after each successful chunk)
- Actions: After every successful chunk (a step turning green, a checkpoint), immediately
  commit and push the working branch to the remote; verify the remote matches local HEAD with
  `git ls-remote`; retry transient failures with backoff (HTTP/1.1 and raised `http.postBuffer`
  on flaky networks); report any remote/local mismatch instead of assuming the push landed.
- Outputs: remote branch / `EXECUTION_LOG.md` entries / fallback bundle
- Done when: either (a) remote HEAD equals local HEAD per `ls-remote`, or (b) push is impossible
  from this environment and the Rule-6/Phase-5 fallback bundle is written and its path reported.
- On failure: retry with backoff up to 3 attempts; then `git bundle create` and report the path.
- Gate: none
- Relevant decisions/claims: D-ADD-004
- Exclusive resources: git remote

### S-ADD-007 Resiliency & HANDOFF restatement (3.4 hook, 3.5)
- Tier: Sonnet
- Profile: All
- Depends on: S-ADD-001
- Actions: Every long-running step in this addendum names its checkpoint cadence, resume command,
  durable-mount contents, and turn-start restaging (done in the steps above). Restate Rules 6, 11,
  12, 13 here verbatim for the implementing agent:

  > **Rule 6 — Everything Is Committed.** No finding, decision, proof, figure spec, or artifact
  > exists unless it is committed to the generation branch. Push cadence: after every successful
  > chunk, immediately push the working branch to the remote, then verify with `ls-remote` that
  > the remote matches local HEAD. Retry transient failures with backoff; report any remote/local
  > mismatch instead of assuming the push landed.
  >
  > **Rule 11 — Lessons.** Whenever the executing agent corrects its own execution, it records a
  > lesson before continuing: `lessons/L-<YYYYMMDDTHHMMSSZ>-<slug>.md` per Appendix A, with
  > trigger, mistake, correction, prevention rule, detection check, classified tags.
  >
  > **Rule 12 — Knowledge Base.** Whenever either agent discovers a new fact about the task's
  > Subject, it adds an item before continuing: `knowledge/K-<YYYYMMDDTHHMMSSZ>-<slug>.md` per
  > Appendix B, with source, version, confidence, and tags including at least one `subject:`.
  >
  > **Rule 13 — Knowledge Store.** Lessons and knowledge items persist across tasks in the
  > knowledge store recorded at intake; they are written only at the end of implementation, after
  > gate G-003, and never contain secrets.

- Outputs: this file (restated rules)
- Done when: this section is present and committed.
- On failure: n/a
- Gate: none
- Relevant decisions/claims: D-ADD-005
- Exclusive resources: none

## Decisions

- **D-ADD-001** — Environment class taken from the protocol's observed table (Kimi cloud =
  ephemeral), corroborated by run evidence (deadline kill of a tool call; persistent-mount
  behavior). Unknowns classify as ephemeral.
- **D-ADD-002** — Durable surface for this run is `/mnt/agents`; the working copy lives at
  `/mnt/agents/output/microtonal-guitar`.
- **D-ADD-003** — Full JUCE VST3 build = environment-blocked dimension (no cmake in sandbox;
  build exceeds container lifetime). CI equivalent to be defined by a future full-protocol run;
  the locally feasible g++ spike of the JUCE-free `TuningEngine` is the substitute verification
  and is labeled as such, not as a plugin build pass.
- **D-ADD-004** — This sandbox has no GitHub credentials, so Rule 6's push cannot land; the
  pre-decided fallback (bundle + path report) executes rather than a silent skip. Recorded in
  `DEVIATIONS.md`.
- **D-ADD-005** — Knowledge store `qualitycoding/knowledge` does not exist (404 at intake);
  per 0.3.7, R0 loads nothing and the run's lessons/knowledge travel on this branch (bundle
  fallback also carries them).

## Implementation record

See `EXECUTION_LOG.md` (one line per step attempt), `DEVIATIONS.md` (all substitutions),
`.checkpoints/state.json` (probe + step pointer), `research/spikes/` (verification spike),
`lessons/` and `knowledge/` (Rules 11–12 items), and the push/bundle record in the log.
