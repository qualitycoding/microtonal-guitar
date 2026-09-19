# Research log — fret note names & free-entry EDO

Run: `gen-20260919T204211Z-fret-note-names` · Base commit: `9644510` (qualitycoding/microtonal-guitar, `main`)

Every claim used by the plan or the frozen tests is listed here with its source. Wiki pages are cited by
permalink (`oldid`) so the oracle cannot drift under the tests.

## A. Naming notes outside 12-TET

**R1. Kite Giedraitis, "Ups and downs notation"**, Xenharmonic Wiki, permalink
<https://en.xen.wiki/index.php?title=Kite%27s_ups_and_downs_notation&oldid=237152> (retrieved 2026-09-19).
Findings used:
- `^` / `v` raise/lower a note by exactly one EDO step; arrows are written *before* the nominal
  (e.g. `^Bb`, not `Bb^`) so chord names stay unambiguous.
- `#`/`b` keep their chain-of-fifths meaning ("raised by 7 fifths"), so in an N-EDO a sharp raises by
  `7f − 4N` steps (the edo's *sharpness*), where `f` is the fifth in steps. Plain (arrow-free) notes
  always form a chain of fifths — the property that makes the labels musically navigable.
- Multiple arrows: `^^` (dup), `^^^` (trup); the wiki's glyphs for 4+ (`v>`, `>`) are **not** adopted
  here; the plan uses repeated ASCII arrows (max label length at 96-EDO is 5 characters, e.g. `vvvvG`).
- 13-EDO and 18-EDO have a descending minor 2nd with their best fifth and are "best notated using the
  2nd best fifth, as 13b and 18b". Flat-N edos (16, 23, 13b, 18b) have two conventions; the plan uses
  the "sharp raises the pitch" row so a `#` is always higher, as a guitarist expects.
- Full absolute-note tables for 12–24-EDO — transcribed into frozen test U4.

**R2. "13edo"**, Xenharmonic Wiki, permalink <https://en.xen.wiki/index.php?title=13edo&oldid=222544>.
- Step size 92.3¢; best fifth 8\13 (738¢), second-best 7\13 (646¢).
- Lists seven notation families; the heptatonic narrow-fifth table (major wider than minor) is used as
  an independent cross-check (frozen test U5). **Conflict noted:** this page also presents a wide-fifth
  (8\13) ups-and-downs table; R1 recommends 13b. The plan follows R1 and records the alternative as a
  possible future option.

**R3. "Musical notation"**, Xenharmonic Wiki <https://en.xen.wiki/w/Musical_notation>.
- Ups and downs notation covers all edos **except 1, 2, 3, 4, 6 and 8** → frozen test U2 and the
  step-number fallback.

**R4. "Alternative symbols for ups and downs notation"**, Xenharmonic Wiki
<https://en.xen.wiki/w/Alternative_symbols_for_ups_and_downs_notation>.
- Stein–Zimmermann quarter-tone accidentals move a note by half the sharpness; perfect (sharp-0) edos
  are 7, 14, 21, 28, 35. Recorded as a *possible display option* for sharp-2 edos (24, 31); not in scope.

**R5. Validation of the spelling rule (this run).** `research/oracle_check.py` implements the spec in
PLAN §3.2 and checks it against every note cell of R1's 12–24 tables and R2's cross-check table.
The first draft of the rule (minimum accidentals + arrows over *any* nominal) failed 14/217 cells
(e.g. it spelled 17-EDO D+5 as `E#` where the wiki allows only `^F/Gb`). Adding the constraint that the
nominal must be one of the two naturals bracketing the pitch fixed all failures: **217/217 pass**,
13-EDO cross-check passes, spellings are unique for every valid edo 5–96 (`research/oracle_output.txt`).
A throwaway C++ reference implementation (not committed) passed all 13 frozen unit test cases
(19,467 assertions), proving the frozen tests are satisfiable.

## B. JUCE / host behaviour

**R6. `AudioProcessorValueTreeState::ComboBoxAttachment`**, JUCE docs
<https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState_1_1ComboBoxAttachment.html>:
combo items are spaced linearly across the parameter's range. A typed value such as "13" is not an item
(`getSelectedId() == 0`), so the attachment cannot represent free entry. Forum confirmation that the
attachment forces a selected item: <https://forum.juce.com/t/no-selection-option-wanted-in-combobox-with-comboboxattachment/32875>
and <https://forum.juce.com/t/possible-bug-in-comboboxattachment/22991>.
→ Plan uses `juce::ParameterAttachment` (listed in `juce_ParameterAttachments.h`,
<https://docs.juce.com/master/juce__ParameterAttachments_8h.html>) with custom text handling.

**R7. Keyboard focus between Live and plugin windows.** A JUCE plugin window that gains focus in Live can
stop Live's computer-MIDI-keyboard keys from reaching Live (<https://forum.juce.com/t/64-bit-vst-eats-keys-in-ableton-live/12801>);
conversely Live does not forward every key to plugin windows (<https://forum.ableton.com/viewtopic.php?t=165524>).
Both reports are old and platform-specific, so behaviour on the user's current Live/Windows setup is
**unverified** → treated as a risk (PREMORTEM F3) with a no-typing fallback and operational tests O3/O4.
Repo fact: `CMakeLists.txt:26` sets `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`.

**R8. doctest 2.4.11** (MIT), vendored single header from
<https://github.com/doctest/doctest/blob/v2.4.11/doctest/doctest.h> so unit tests build offline.

## C. Repository findings (base 9644510) that affect correct labelling

| ID | Location | Finding | Why it matters for labels |
|----|----------|---------|---------------------------|
| C1 | `TuningEngine.h:27` | `openStringMidiNote(s) = 40 + 5s` → string 4 is MIDI 60 (C4) | Open-string *frequency* for string 4 is 246.94 Hz (B3), `PluginProcessor.cpp:92`. Note routing and pitch disagree on the B string; a label derived from MIDI would be wrong. |
| C2 | `PluginProcessor.cpp:92`, `TuningEngine.cpp:23` | Only 6 open frequencies; `stringCount` allows 12 | `getFrequency(s>5)` reads past the vector. The fretboard *draws* up to 12 strings, so per-string labels would trigger the out-of-bounds read. |
| C3 | `PluginProcessor.cpp:114` | `currentNumStrings` is never updated | MIDI routing always uses 6 strings regardless of the slider. |
| C4 | `PluginProcessor.cpp:79` | `refFreq` is stored in state but never used for pitch | Labels anchored on `refFreq` would drift from what sounds. |
| C5 | `PluginEditor.h:83` | Octave-dot logic hard-codes 12/24/31 | Must follow the free EDO value. |
| C6 | `PluginProcessor.cpp:236`, `PluginEditor.cpp:20` | Fret count and tuning combo are hard-wired to 4 presets via `ComboBoxAttachment` | Replaced by `fretCountFor(N)` and a custom attachment (R6). |
| C7 | `PluginProcessor.cpp:33` vs `:21-23` | `musicalKey` listener removed but never added (already known) | Out of scope; untouched. |
| C8 | `processBlock`, `MicrotonalVoice::updateFrequency` | One MIDI key = one EDO step per string, but string offsets are 5 keys (12-TET fourths) | In non-12 EDOs pitch is not monotonic across keys. Labels will faithfully show this. Out of scope; logged as backlog B1. |

## D. Numbers checked in this run (oracle_output.txt)
- 31-EDO: open low E (82.41 Hz) → nearest lattice note **E2, +3.3¢** (step −75 from A4).
- 13-EDO: open low E → step −31 → spelled **Fb2, −38.4¢** (12-TET E is not on the 13-EDO lattice).
- 5-limit major third above C4 → **E, −13.7¢**.

## E. Instrument tunings (added in r2)

**R9. "Standard tuning"**, Wikipedia <https://en.wikipedia.org/wiki/Standard_tuning> and **"Bass guitar tuning"**
<https://en.wikipedia.org/wiki/Bass_guitar_tuning>:
- Four-string bass standard tuning is E1 A1 D2 G2 in fourths — the guitar's lowest four strings an octave
  lower (MIDI 28, 33, 38, 43; E1 = 41.203 Hz).
- Five- and six-string basses usually add a **low B0** (and a high C3 on six strings), which keeps
  all-fourths but changes where the lowest string sits → plan decision D5 / backlog B6.
- Guitar standard tuning E2 A2 D3 G3 B3 E4 (MIDI 40 45 50 55 59 64) has a major third between G and B,
  so "guitar" and "all fourths" differ only on strings 5 and above.
