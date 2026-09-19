# Pre-mortem — "It's March 2027 and the note-name release failed"

Severity: **Critical** = crash, data loss or wrong musical output shipped · **High** = core feature
unusable for the owner's workflow · **Medium** = degraded/confusing · **Low** = cosmetic.
Each iteration assumes the plan *as hardened by the previous iteration* was followed perfectly.

## Iteration 1 (plan v0: "add NoteNaming, draw names, make combo editable")

| # | Failure story | Root cause | Sev | Hardening added to PLAN |
|---|---------------|------------|-----|-------------------------|
| F1 | Labels on the B string read `C` while it sounds B; in 31-EDO labels disagree with the tuner | Label derived from MIDI note; `openStringMidiNote` ≠ open frequency (C1); `refFreq` ignored (C4) | Critical | §3.1 single source of truth; labels from engine frequency; tests I7, I8 |
| F2 | Plugin crashes when the fretboard labels 7+ strings | `openStringFrequencies` has 6 entries, slider allows 12 (C2); labels are the first code to call `getFrequency` for every drawn string | Critical | S3 bounds + 12-entry table; test I6 |
| F3 | Owner types `13`, then QWERTY notes stop playing in Live (or typing never reaches the box) | Host-dependent key routing (R7) | High | Focus released on commit; ▲/▼ + wheel path needs no keyboard; `EDITOR_WANTS_KEYBOARD_FOCUS` unchanged; O3, O4 |
| F4 | Typing `13` snaps back to `12-TET` | `ComboBoxAttachment` maps only list items (R6) | High | Custom `ParameterAttachment` (§3.3, S5); I2 |
| F5 | Old Live sets reopen in 12-TET | `tuningType` removed without migration | High | Migration + explicit `updateGuitarState()`; I4, O6 |
| F6 | Audio dropouts when all-frets labels are on | UI took the engine mutex once per label per frame (12×96×30 ≈ 35k/s) contending with the audio thread | High | `getSnapshot()` once per paint + label cache (§3.3) |
| F7 | Names are wrong for some EDOs | Naming rule assumed, not verified | High | Oracle check vs wiki 217 cells (R5), property test U7, fallback U2 |
| F8 | Every fret in 13-EDO shows `−38¢`; owner files it as a bug | Open strings are 12-TET pitches, not on the 13-EDO lattice | High | Gate G0 decision D1 before any code; O10 documents expected display |
| F9 | Labels unreadable above ~40-EDO | Text wider than fret slots | Medium | Culling (L2); active notes always labelled |
| F10 | `¢` renders as garbage | UTF-8 literal passed to `juce::String(const char*)` | Medium | §3.3 mandates `String::fromUTF8`; U10 fixes the byte sequence |
| F11 | Tests can't build offline | Test framework fetched at configure time | Medium | doctest vendored (R8) |
| F12 | Owner thinks labels are wrong because adjacent keys jump around in pitch | Existing key mapping (C8) | Medium | Documented as B1; labels are truthful |

## Iteration 2 (plan v1 = v0 + all hardening above)

| # | Failure story | Root cause | Sev | Hardening |
|---|---------------|------------|-----|-----------|
| F13 | Frozen JUCE tests don't compile; executor edits assertions to "fix" them | Integration tests were compile-unverified at freeze | High | S4 rule: harness-only fixes allowed and logged; assertion edits trigger Reset; hashes checked at every resume (§5) |
| F14 | Labels stale after moving the A4 slider | Cache key omitted `refFreq` | Medium | Cache key = full snapshot + mode (§3.3) |
| F15 | Migration code runs but engine stays in 12-TET | `replaceState` sets values equal to defaults → no listener callback | High | Explicit `updateGuitarState()` after migration (S3); I4 exercises the path |
| F16 | Host automation changes EDO while the box is being edited; text shows old value, commit overwrites automation | Attachment updates text unconditionally / edit commits blindly | Medium | Attachment skips text refresh while editing; commit only on explicit Enter |
| F17 | Held notes jump pitch or hang when EDO is automated | Voices recompute fret on pitch-bend; note-off uses the stored string map | Medium | O7; note-off path already keyed by `noteToStringMap` (unchanged) |
| F18 | Enabling 12 strings changes which string plays a note | `currentNumStrings` now follows the slider (C3 fix) | Medium | Intended; README note (S7) |
| F19 | Executor resumes on a machine with a different JUCE and silently rebuilds from scratch | No recorded environment | Low | state.json records JUCE path/version and commit SHA |

## Iteration 3 (plan v2)
Walked the same six-month scenario across build, host, data, threading, UX and research assumptions.
No new Critical or High failure modes were found. Remaining risks:

| # | Residual risk | Sev | Why accepted |
|---|---------------|-----|--------------|
| R-a | Live on the owner's current Windows build may still intercept some keys in text fields (R7 reports are old) | Medium | A fully keyboard-free path (O4) exists; O3 detects it before release |
| R-b | 13b vs wide-fifth 13-EDO names may not match the owner's taste | Low | D3 gate + backlog B4 |
| R-c | Automation lanes on the removed `tuningType` are lost | Low | D4 gate; documented |
| R-d | The 217-cell oracle only covers 12–24-EDO; 25–96 rely on the rule + property tests (U7) | Medium | Rule matches every published table checked; U6 pins 31-EDO; spellings are unique and periodic for all N |

## Iteration 4 (r2 — plan v3 adds Instrument mode)

| # | Failure story | Root cause | Sev | Hardening |
|---|---------------|------------|-----|-----------|
| F20 | In Bass mode the dot sits on the wrong string and the label disagrees with the sound | Router or voice still calls the one-argument (Guitar) `openStringMidiNote` while the engine uses Bass | High | All three read mode from one engine snapshot (§3.1); I11 checks router *and* voice (`getActiveNotes`) in both modes |
| F21 | Old Live sets open in Bass or with shifted open strings | New param default or migration wrong | High | Default Guitar; I12 legacy + round-trip |
| F22 | Switching mode mid-note leaves a stuck voice | Note-off looks up a string computed with the old table | Medium | Note-off already keyed by `noteToStringMap` (unchanged); O13 |
| F23 | Bass E1 sounds weak or clicky in the Karplus-Strong voice | 41 Hz fundamental, averaging low-pass designed around guitar range | Medium | O12 level/click check; tuning of the voice is backlog, not this feature |
| F24 | Owner expected a low-B 5-string bass | "All fourths" read literally as upward extension | Medium | Recorded as D5 assumption, surfaced before S3; B6 |

No new Critical or High remain after hardening. Residual risks R-a…R-d unchanged, plus F23/F24 (Medium).

**Stop condition met:** 0 Critical, 0 High open.
