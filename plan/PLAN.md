# Execution plan — fret note names & free-entry "notes per octave"

Branch `gen-20260919T222430Z-fret-note-names-r2` (revision r2, built on r1 `gen-20260919T204211Z-fret-note-names`) · base `9644510` · status: **PLAN ONLY — not executed**

> r2 change: owner answered G0 (below). D1 introduced a new requirement — an **Instrument mode (Guitar / Bass)** — so the protocol was re-run from Phase 0. r1 frozen files are unchanged (hash-verified); r2 adds frozen tests I9–I12 and O11–O13.

## 1. Goal and definition of done
1. The player can always see **which note is sounding**: the gold active-note dot carries its name, and a
   readout shows name, octave, cents offset and step (e.g. `vD3  −2¢  step −30 of 31`).
2. Optionally, every fret on every string is labelled when there is room ("Show all note names" toggle).
3. Names outside 12-TET use **Kite's ups-and-downs notation** (research R1–R5); 12-TET output is
   exactly today's `C C# D … B`.
4. The tuning control accepts **free numeric entry** (`13`, `13edo`, `19-TET`…) in addition to presets,
   range 1–96, plus ▲/▼ steppers and mouse-wheel so no typing is ever required.
5. An **Instrument** selector: Guitar = standard tuning E2 A2 D3 G3 B3 E4; Bass = all fourths from E1 (E1 A1 D2 G2 C3 F3 …).
6. Done = all frozen unit (U1–U11, L1–L2) and integration (I1–I12) tests green, operational O1–O13 PASS.

## 2. Owner decisions (G0 — answered 2026-09-19)
| ID | Question | Answer | Effect |
|----|----------|--------|--------|
| D1 | Open-string tuning | New **Instrument mode**: Guitar = standard E A D G B E; Bass = all fourths. Open strings stay at 12-TET pitches (not snapped to the EDO) | Labels show honest cents in EDOs that miss 12-TET pitches (O10); tests I9–I12 |
| D2 | Should A4 retune the instrument? | **Yes**, the whole instrument | I7, I10 |
| D3 | 13/18-EDO naming | **13b/18b** (second-best fifth) | U1, U4, U5 |
| D4 | Lose automation lanes on removed `tuningType` | **Accepted** | README note (S7) |
| D5 | Bass strings 5–12: extend upward in fourths from E1 (E A D G C F …) or add a low B first (B E A D G, common on 5-string basses)? | **Assumed upward** from the literal "all fourths" answer — owner may override before S3 | I10 pins the upward table; overriding it is a Reset-Rule event |

## 3. Design

### 3.1 Architecture
- New pure-C++17 module `NoteNaming.{h,cpp}` (contract frozen at `plan/api/NoteNaming.h`), no JUCE,
  unit-tested by `tests/unit`.
- Parameters: remove `tuningType`; add `tuningMode` (Choice: `EDO`, `5-Limit Just`) and `edoSteps`
  (`AudioParameterInt` 1..96, default 12). Migration of legacy state in `setStateInformation`.
- `TuningEngine` gets `GuitarState getSnapshot() const` (one lock per paint, not one per label).
- One source of truth for open strings: `TuningEngine::openStringMidiNote(s, InstrumentMode)` —
  Guitar `{40,45,50,55,59,64,69,74,79,84,89,94}`, Bass `28 + 5s`; the one-argument overload stays as the
  Guitar table for compatibility. Frequency = `refFreq · 2^((midi−69)/12)` (fixes C1, C2, C4).
  New parameter `instrumentMode` (Choice `Guitar`,`Bass`, default Guitar). The router in `processBlock`,
  `MicrotonalVoice::updateFrequency` and the UI all read the mode from the **same engine snapshot** —
  never the one-argument overload. `currentNumStrings` updated in
  `updateGuitarState` (fixes C3).
- Labels are computed from the **frequency the engine will actually produce** — never from the MIDI
  note — via `nameFrequency(hz, refFreq, N)`. Just intonation uses N=12 for naming plus cents.

### 3.2 Spelling rule (validated 217/217 against the wiki, R5)
For N-EDO: `f = round(N·log2 1.5)`; if `3N − 5f < 0` use `f − 1`; invalid if `2f − N ≤ 0` or N ∈ {1,2,3,4,6,8}
(→ step-number labels). Sharpness `s = |7f − 4N|`. Natural positions (unwrapped): C=0, D=2f−N,
E=4f−2N, F=N−f, G=f, A=3f−N, B=5f−2N, C′=N. For step r: if r equals a natural's position, return it (latest
letter wins on ties). Otherwise consider only the two bracketing naturals, with accidentals −2..+2
(none if s=0), and arrows `u = r − (pos + a·s)`. Pick the minimum of
`(|u|+|a|, |u|, flats-after-sharps, downs-after-ups, lower-letter-first)`.

### 3.3 UI
- Tuning control: editable `ComboBox` bound by a custom `juce::ParameterAttachment` (R6 — the stock
  `ComboBoxAttachment` cannot hold typed values). Items: `12-TET`, `19-EDO`, `22-EDO`, `24-TET`, `31-EDO`,
  `41-EDO`, `53-EDO`, `5-Limit Just`. On commit → `parseTuningText`; invalid → revert + tooltip "1–96".
  Focus is released on Enter/Escape/click-away (`giveAwayKeyboardFocus`) so Live's QWERTY keyboard keeps
  working (R7). `EDITOR_WANTS_KEYBOARD_FOCUS` stays `FALSE`.
- ▲/▼ buttons and mouse-wheel step `edoSteps` by 1.
- Active-note dot: name drawn inside (radius already 5–12 px; font sized to fit, else drawn above the dot).
- Readout label under the controls: `name+octave  cents  step k of N`.
- All-frets labels: small grey text centred in each fret slot when `shouldDrawFretLabel(slot, textW)`.
- Octave dots at multiples of N; the 12-TET 3/5/7/9 inlays stay 12-only.
- Label text cache keyed on the full snapshot (N, mode, refFreq, stringCount, open strings); rebuilt only
  when the key changes, never per frame.
- Text via `juce::String::fromUTF8` (the `¢` is UTF-8).

## 4. Steps (minimum model tier in brackets; ✔ = checkpoint written after the step)

**G0 [Fable]** Present D1–D4 to the owner; record answers in `.checkpoints/state.json.decisions`. ✔

**S1 [Sonnet]** Copy `plan/api/NoteNaming.h` to repo root; add empty `NoteNaming.cpp`; wire
`tests/unit/CMakeLists.txt`. Confirm tests *compile and fail* (red). ✔ `phase: S1_red`

**S2 [Opus]** Implement `NoteNaming.cpp` per §3.2 until U1–U11, L1–L2 pass. Tests may not be edited.
If a test looks wrong → **stop, Reset Rule** (Phase 0). ✔ after each green test case (resume point = first
failing case).

**S3 [Opus]** `TuningEngine`: open-string table + `refFreq` anchor, bounds checks, `getSnapshot()`.
Processor: new params, listeners for `edoSteps`/`tuningMode`, `currentNumStrings` update,
`getFretCount()` → `fretCountFor`, `instrumentMode` parameter + listener, mode-aware routing in
`processBlock` and `MicrotonalVoice::updateFrequency`, legacy migration after `apvts.replaceState` followed by an explicit
`updateGuitarState()` (a listener does not fire if the value is unchanged). ✔

**S4 [Sonnet]** Add CMake target `MicroGuitarIntegrationTests` (console app, `juce_add_console_app`,
same sources minus editor if needed, plus `tests/integration/*.cpp`). Build; run; I1–I12 green.
Harness-only compile fixes (includes, namespaces) allowed and logged under `test_harness_fixes`; any
change to an assertion = Reset Rule. ✔

**S5 [Opus]** Instrument selector (plain `ComboBox` + stock attachment is fine — two fixed items) and custom tuning control (`EdoControl` component: editable combo + steppers +
`ParameterAttachment`), focus release, validation tooltip. ✔

**S6 [Sonnet]** Fretboard: active-note names, readout, all-frets toggle with culling, octave dots from N,
snapshot-per-paint + label cache. ✔

**S7 [Haiku]** README: notation explanation (link R1), new control, D4 automation note. ✔

**S8 [Sonnet]** Release build (Ninja, `-DCMAKE_BUILD_TYPE=Release`, per project learnings); run unit +
integration tests; install VST3. ✔

**S9 [Human + Sonnet]** Run O1–O13 in Live; record results. Any FAIL → fix in S5/S6 without touching
frozen tests, re-run the full suite. ✔ `phase: done`

## 5. Checkpoint / resume protocol
- `.checkpoints/state.json` is rewritten after every ✔ with: `phase`, `last_green_test`, `decisions`,
  `test_harness_fixes`, `operational_results`, `frozen_hashes_verified` and the commit SHA.
- On start, an executor reads the file, re-verifies frozen hashes (`sha256sum -c tests/FROZEN.sha256`),
  checks out the recorded SHA, and resumes at the recorded phase. Hash mismatch → halt (frozen suite was
  modified).
- Each step commits on its own; rate-limit or crash loses at most one step.

## 6. Out of scope / backlog
- B1: non-monotonic key→pitch mapping in non-12 EDOs (C8) — labels will make it visible; fixing it is a
  separate design (e.g. per-string step offsets derived from the EDO's fourth).
- B2: `musicalKey` listener bug (C7); B3: optional Stein–Zimmermann glyphs for sharp-2 edos (R4);
  B4: wide-fifth 13-EDO option (D3); B5: snap-to-lattice open strings (D1); B6: low-B bass layout (D5);
  B7: default string count per instrument (e.g. Bass → 4) — today `stringCount` stays independent.

See `plan/PREMORTEM.md` for the failure analysis that shaped §3–§5.
