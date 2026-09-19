# Operational tests — revision r3 (FROZEN) — instrument presets

| ID | Setup | Action | Pass criterion |
|----|-------|--------|----------------|
| O14 | 12-TET, each instrument at its min and max string count | Play every open string | Labels and a tuner agree with the tables in U12 (e.g. 7-string guitar B1…E4, 6-string bass B0…C3, cello C2 G2 D3 A3) |
| O15 | Guitar, move the Strings slider 4→12 | Watch the fretboard | Strings stop at 8; the UI shows the instrument's maximum (e.g. "8 max") instead of silently ignoring the slider |
| O16 | 6-string bass and 5-string cello | Play the lowest open string 5 s | Audible fundamental, no clicks, level within 6 dB of guitar E2 |
| O17 | Hold a note, switch instrument, release | — | Note stops (no stuck voice); next note uses the new layout |
| O18 | Violin / Mandolin preset | Play open E5 and fret 12 on it | Pitch correct; readout names match (E5, E6 in 12-TET) |
