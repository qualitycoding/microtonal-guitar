# Operational test suite — FROZEN

Manual, run in Ableton Live on Windows with a Release VST3 build. Each test records PASS/FAIL,
Live version, and date in `.checkpoints/state.json` → `operational_results`. Any FAIL blocks release.

| ID | Setup | Action | Pass criterion |
|----|-------|--------|----------------|
| O1 | Fresh instance, 12-TET | Play A2 (QWERTY) | Gold dot on A string open shows **A**; readout shows `A2`, no cents |
| O2 | Fresh instance | Click tuning box, type `13`, press Enter | Readout/fretboard switch to 13 steps, 26 frets, labels in ups-and-downs notation |
| O3 | After O2 | Immediately press QWERTY note keys (computer MIDI keyboard ON) | Notes sound; keystrokes are **not** swallowed by the plugin (focus released after Enter/Escape/click-away) |
| O4 | Any | Change EDO using only the ▲/▼ stepper buttons and mouse wheel (no typing) | Every value 1–96 reachable; label and pitch update |
| O5 | Any | Type `abc`, `0`, `97`, `13.5` then Enter | Box reverts to the previous valid value; no crash; tooltip explains the range |
| O6 | Live set saved with commit 9644510 in 31-EDO | Open with new build | Instrument loads in 31-EDO |
| O7 | Automation lane on "Steps per Octave" | Automate 12→31 during playback | No crash, no stuck notes; held notes keep sounding until released |
| O8 | 12 strings, 96-EDO | Play chords across all strings for 60 s | No crash; UI stays at ≥ 25 fps (JUCE perf counter log) |
| O9 | Windows display scale 100 %, 150 %, 200 % | Open editor | Labels legible, not overlapping frets (culling active at 96-EDO) |
| O10 | 13-EDO, default open strings | Play open strings | Labels show the honest name + cents (e.g. `Fb −38¢` on low E) — see plan decision D1 |
