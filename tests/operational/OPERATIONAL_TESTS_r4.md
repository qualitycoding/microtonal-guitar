# Operational tests — revision r4 (FROZEN) — instrument presets

| ID | Setup | Action | Pass criterion |
|----|-------|--------|----------------|
| O14 | 12-TET, each instrument at its min and max string count | Play every open string | Labels and a tuner agree with U12 (e.g. 10-string guitar G#0…E4, 15-string bass F#0…E6, cello C2 G2 D3 A3) |
| O15 | Guitar, move Strings 4→15 | Watch the fretboard | Strings stop at 10 and the UI shows "10 max"; switching to Bass shows all 15 |
| O16 | 10-string guitar, 8-string bass, 5-string cello | Play the lowest open string 5 s | Audible fundamental, no clicks, level within 6 dB of guitar E2 |
| O17 | Hold a note, switch instrument, release | — | Note stops (no stuck voice); next note uses the new layout |
| O18 | Violin / Mandolin preset | Play open E5 and fret 12 on it | Pitch correct; readout names match (E5, E6 in 12-TET) |
| O19 | 15-string bass | Play a 15-note chord, one note per string (MPE channels 2–16) | All 15 notes sound; no voice stealing; fretboard fits 15 strings legibly at 100 % and 150 % scaling |
