# K-20261101T200000Z-guard-seam-pattern

- **applies:** all
- **domain:** domain:audio
- **tech:** tech-cpp, tech-juce

For guard remediations (zero/inf/NaN inputs), extract the validation into a pure,
header-only seam with no framework includes (C++17 only, e.g.
`FrequencyGuard.h::safeDelayLength(sampleRate, targetFreq, bufferSize)` returning
0.0 on any violation).

**Why:** red/green test evidence then compiles and runs host-independently — no
JUCE, no CMake, no DAW — so the red record is reproducible in any sandbox and in
CI's fast gate, and the same seam is callable from both the audio thread and tests.

**Contract to lock in tests:** on invalid input the voice must enter a defined
silent state (no NaN/inf reaches the DAC), and the next valid note must fully
reinitialize the voice state. Mirror the render loop's silent-state check in the
test simulation, or the test itself can segfault on inf delay lengths.

**Source:** microtonal-guitar issue #5 remediation (PR #14, 2026-11-01),
boundary tests U-B1..U-B4 and integration test I-B1.
