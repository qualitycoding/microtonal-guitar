## Microtonal MPE Guitar VST

This is a JUCE-based VST3/Standalone plugin designed to bring authentic microtonal guitar playing to your DAW. By leveraging MIDI Polyphonic Expression (MPE) and physical modeling, this instrument accurately tracks per-string pitch bends without locking you into standard Western temperaments.

### Core Features

* **Any Equal Division of the Octave, 1-96, entered freely:** type a number (`13`, `19edo`, `31 TET`...) into the tuning box, use the +/- steppers, or pick a preset. 5-Limit Just Intonation is available as a separate mode.


* **Note names in every tuning, not just 12-TET:** frets and the currently-sounding note are labelled using [Kite's ups-and-downs notation](https://en.xen.wiki/w/Ups_and_downs_notation) - `^` raises a note by one step of the current EDO, `v` lowers it, and sharps/flats keep their usual chain-of-fifths meaning. In EDOs where that notation doesn't apply (1, 2, 3, 4, 6, 8 steps), frets are labelled by step number instead. The readout under the fretboard also shows the cents offset from that name and which scale step you're on.


* **Instrument presets:** Guitar, Bass, Violin/Mandolin, Viola/Mandola and Cello, each with its own realistic open-string layout and string-count range (see table below). Fewer strings than standard drop the lowest string(s); guitar and the fifths-tuned presets never add a string above their standard top string, while bass can extend upward past its 6-string layout into the guitar's register.

  | Instrument | Strings | Standard tuning (low to high) |
  |---|---|---|
  | Guitar | 4-10 | E2 A2 D3 G3 B3 E4 (7th string +B1, 8th +F#1) |
  | Bass | 4-15 | E1 A1 D2 G2 (5th string +B0, 6th +C3, 7th +F3, then extends upward) |
  | Violin / Mandolin | 4-5 | G3 D4 A4 E5 |
  | Viola / Mandola | 4-5 | C3 G3 D4 A4 |
  | Cello | 4-5 | C2 G2 D3 A3 |


* **Karplus-Strong Synthesis:** Generates audio using physical string modeling, utilizing noise bursts and fractional delay lines to ensure perfect microtonal pitches without rounding errors.


* **Smart String Allocation:** The engine acts like a virtual left hand, automatically assigning incoming MIDI notes to the highest-pitched open string at or below the note, prioritizing the lowest possible fret.


* **Dynamic Visual Fretboard:** Draws the strings, frets and inlay markers for any string count and any EDO, highlights active notes in real-time with their name inside the dot, optionally labels every fret ("Show all note names"), and automatically scrolls to follow your playing position.


* **Adjustable Reference & Key:** The A4 reference frequency (400Hz-480Hz) retunes the whole instrument, not just the display. Also select a base musical key (this dropdown is not yet wired to anything - see Known limitations).


### Architecture Overview

The plugin deliberately separates the mathematical tuning logic from the audio synthesis. The `TuningEngine` serves as the brain, handling the specific mathematical ratios and logarithmic equations required for Equal Divisions of the Octave (EDO) or Just Intonation. It holds one `GuitarState` snapshot - open-string frequencies *and* their nearest 12-TET MIDI notes, computed together from the current instrument and reference frequency - so note routing, fret labels and pitch can never disagree with each other.

Two small, JUCE-independent modules do the rest of the new work and are unit-tested on their own: `NoteNaming.{h,cpp}` (the ups-and-downs spelling rule and frequency-to-name lookup) and `Instruments.{h,cpp}` (the per-instrument open-string tables).

When a note is played, the `MicrotonalVoice` generates a short noise excitation and feeds it into a feedback delay buffer. To keep the visual interface smooth without causing audio dropouts, the UI reads one locked snapshot of the tuning state per repaint (rather than once per fret label) and caches the computed note-name strings, rebuilding the cache only when the tuning, instrument or reference frequency actually changes.

### Building and testing

You need CMake 3.22 or later and a C++17 or newer compiler. The build downloads JUCE 9.0.4 itself (pass
`-DMICROGUITAR_JUCE_TAG=...` to change it, or `-DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE` to use a local copy). Formats are VST3 and Standalone.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The tests are separate CMake projects, each built the same way and run with `ctest`: `tests/unit`,
`tests/tuning_engine`, `tests/boundary` and the JUCE-based `tests/integration`. CI (`.github/workflows/build.yml`) runs
the first three on Ubuntu and checks the frozen test manifest (`tests/FROZEN.sha256`).

### Known limitations / backlog

* The "Musical Key" dropdown's parameter listener was never wired up in the original code (it has no audible or visual effect); out of scope for this change.
* Adjacent MIDI keys are five semitones apart per string (a 12-TET fourth) regardless of the chosen EDO, so pitch is not monotonic across the keyboard in most non-12 tunings. Note *names* are correct wherever a note lands; a per-EDO key mapping is a separate future change.
* Loading a project saved with an older version of this plugin migrates its tuning preset (12/24/31-TET or 5-Limit Just) to the new controls and sets the instrument to Guitar, but any DAW automation lane that was targeting the old "Tuning Type" parameter will no longer have anything to control - the plugin doesn't know how to remap automation.
* Open strings always sound at their standard 12-TET pitches, even in EDOs that don't include those exact pitches (e.g. the low E string in 13-EDO reads as `Fb2 -38¢`, not a clean `E`). This is intentional (see `plan/PLAN.md` decision D1), not a bug.
* Cello, Viola/Mandola and Violin/Mandolin are plucked (Karplus-Strong), so they won't sound bowed.

See `plan/PLAN.md`, `plan/PREMORTEM.md` and `research/RESEARCH_LOG.md` for the full design history, citations and risk analysis behind these features.

### Versioning and releases

The release version lives in a single place: the `VERSION` file at the repo root (`MAJOR.MINOR.PATCH`, semver). CMake reads it into the project version and passes it to the plugin binaries, so the VST3/Standalone builds always report the same version as the tag they were built from.

Release channel: every push to `main` (and every PR) produces CI build artifacts for Windows, Linux and macOS — those are dev/nightly binaries. Pushing a tag of the form `vX.Y.Z` additionally runs a `release` CI job that collects the three platform artifacts and publishes a GitHub Release with per-platform zips and auto-generated notes. To cut a release: set `VERSION` to the new number, commit, tag `vX.Y.Z` at that commit and push the tag.

---


Copyright 2026 Quality Coding

Licensed under the Apache License, Version 2.0
