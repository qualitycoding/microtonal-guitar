# FROZEN test suite

Frozen at 2026-09-19T20:42Z on branch `gen-20260919T204211Z-fret-note-names`.
Status: **FROZEN** — implementers may not modify, skip, disable or weaken any file listed in
`tests/FROZEN.sha256` (including the interface contract `plan/api/NoteNaming.h`).

Verify before every step:  `sha256sum -c tests/FROZEN.sha256`

| Suite | File | Cases | Verified at freeze |
|-------|------|-------|--------------------|
| Unit (no JUCE) | `unit/test_note_naming.cpp` | U1–U11 | Compiles; passes against a throwaway reference impl (19,467 assertions) |
| Unit (no JUCE) | `unit/test_fret_layout.cpp` | L1–L2 | Same |
| Integration (JUCE) | `integration/TuningParameterTests.cpp` + runner | I1–I8 | **Compile-unverified** (no JUCE in planning sandbox) — see PLAN S4 harness-fix rule |
| Operational (manual, Live) | `operational/OPERATIONAL_TESTS.md` | O1–O10 | n/a |

Reset Rule: if any frozen test is believed invalid, stop, record the reason in
`.checkpoints/state.json.reset_log`, and restart the protocol at Phase 0. Do not patch the test.
