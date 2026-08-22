## Microtonal MPE Guitar VST

This is a JUCE-based VST3/Standalone plugin designed to bring authentic microtonal guitar playing to your DAW. By leveraging MIDI Polyphonic Expression (MPE) and physical modeling, this instrument accurately tracks per-string pitch bends without locking you into standard Western temperaments.

### Core Features

* **Customizable Tuning Systems:** Switch instantly between 12-TET, 24-TET, 31-EDO, and 5-Limit Just Intonation directly from the UI.


* **Dynamic String Count:** Adjust the instrument to have anywhere between 4 and 12 strings.


* **Karplus-Strong Synthesis:** Generates audio using physical string modeling, utilizing noise bursts and fractional delay lines to ensure perfect microtonal pitches without rounding errors.


* **Smart String Allocation:** The engine acts like a virtual left hand, automatically assigning incoming MIDI notes to the highest-pitched open string at or below the note, prioritizing the lowest possible fret.


* **Dynamic Visual Fretboard:** Features a responsive UI component that draws the strings, frets, and inlay markers, highlights active notes in real-time, and automatically scrolls to follow your playing position.


* **Adjustable Reference & Key:** Fine-tune your A4 reference frequency (from 400Hz to 480Hz) and select your base musical key.



### Architecture Overview

The plugin deliberately separates the mathematical tuning logic from the audio synthesis. The `TuningEngine` serves as the brain, handling the specific mathematical ratios and logarithmic equations required for Equal Divisions of the Octave (EDO) or Just Intonation.

When a note is played, the `MicrotonalVoice` generates a short noise excitation and feeds it into a feedback delay buffer. To keep the visual interface smooth without causing audio dropouts, the UI and audio threads communicate safely using atomic variables, ensuring the dynamic fretboard always reflects exactly what the synthesizer engine is doing.


---


Copyright 2026 Quality Coding

Licensed under the Apache License, Version 2.0