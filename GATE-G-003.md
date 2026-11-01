# GATE-G-003 — Lessons & Knowledge Push Request

Per plan S-KNOW and deviation rule D-005: nothing is pushed to any external
lessons/knowledge store without owner sign-off.

## Proposed items to push to the shared/proposed store

### Lessons
1. `lessons/L-20261101T124000Z-sandbox-recycle-mid-build.md` — sandbox recycles within ~15 min; anything in /tmp is lost; stage toolchains/JUCE on the durable mount.
2. `lessons/L-20261101T130000Z-mount-build-cache-and-split-units.md` — JUCE 9 module layout (split `juce_gui_basics_2..5`, `juce_audio_processors_headless`, amalgamated C units); FUSE mount is noexec and the assembler cannot write .o directly to it — compile to /tmp, cache objects on the mount.

### Knowledge
1. `knowledge/K-20261101T-guard-seam-pattern.md` — for guard remediations, extract a pure header-only validation seam (C++17, no framework headers); red evidence then compiles/runs host-independently.
2. `knowledge/K-20261101T-frozen-manifest-refreeze.md` — frozen test manifests must be re-frozen in the same PR that changes frozen files; recommend a CI manifest-check job.

## Requested decision (pick one)
- [ ] Push the four items above to the `proposed/` area of the lessons/knowledge store (state the target repo/path).
- [ ] Push to an alternate target: __________
- [ ] Do not push; keep them in-repo only.

No push will be performed until a selection is made.
