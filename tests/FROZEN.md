# FROZEN test suite

Frozen at 2026-09-19T20:42Z on branch `gen-20260919T204211Z-fret-note-names`.
Status: **FROZEN** — implementers may not modify, skip, disable or weaken any file listed in
`tests/FROZEN.sha256` (including the interface contract `plan/api/NoteNaming.h`).

Verify before every step:  `sha256sum -c tests/FROZEN.sha256`

| Suite | File | Cases | Verified at freeze |
|-------|------|-------|--------------------|
| Unit (no JUCE) | `unit/test_note_naming.cpp` | U1–U11 | Compiles; passes against a throwaway reference impl (19,467 assertions) |
| Unit (no JUCE) | `unit/test_fret_layout.cpp` | L1–L2 | Same |
| Integration (JUCE) | `integration/TuningParameterTests.cpp` + runner | I1–I7 (I6 re-frozen r4; I8 retired) | **Compile-unverified** (no JUCE in planning sandbox) — see PLAN S4 harness-fix rule |
| Operational (manual, Live) | `operational/OPERATIONAL_TESTS.md` | O1–O10 | n/a |
| Unit (no JUCE) — r4 | `unit/test_instruments.cpp` | U12–U15 | Passes against throwaway reference impl (all 17 unit cases, 19,926 assertions) |
| Integration (JUCE) — r4 | `integration/InstrumentTests.cpp` | I13–I16 | **Compile-unverified** |
| Operational — r4 | `operational/OPERATIONAL_TESTS_r4.md` | O14–O19 | n/a |

Reset Rule: if any frozen test is believed invalid, stop, record the reason in
`.checkpoints/state.json.reset_log`, and restart the protocol at Phase 0. Do not patch the test.

Revision r2 (2026-09-19T22:24Z, branch `gen-20260919T222430Z-fret-note-names-r2`): r1 files re-verified unchanged; r2 files appended to `FROZEN.sha256` and frozen.

Revision r3 (2026-09-19T22:40Z, branch `gen-20260919T224047Z-fret-note-names-r3`): **Reset Rule applied.** I8 and r2's I9–I12 / O11–O13 were invalidated by the owner's tuning clarification and moved to `tests/retired/` (not built). I6 replaced in place. `FROZEN.sha256` regenerated for the full r3 suite.

Revision r4 (2026-09-19T22:49Z, branch `gen-20260919T224942Z-fret-note-names-r4`): **Reset Rule applied.** Owner's A1/A3 answers invalidated U12–U15, I6, I14 and O14–O18. r3 versions preserved in `tests/retired/r3/`. `FROZEN.sha256` regenerated.
