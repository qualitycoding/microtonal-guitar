---
id: K-20261101T200000Z-guard-seam-pattern
statement: "For guard remediations (zero/inf/NaN inputs), extract the validation into a pure header-only seam with no framework includes (C++17 only, e.g. safeDelayLength(sampleRate, targetFreq, bufferSize) returning 0.0 on any violation): red/green evidence then compiles host-independently, and the same seam is callable from both the audio thread and tests."
status: current
supersedes: []
tags: [subject:guard-seam-pattern, domain:audio, tech:juce, tech:cpp, phase:verification]
applies_to_version: "microtonal-guitar PR #14 (fix/issue-5-zero-freq-guard)"
source: { url: "", doi: "", title: "FrequencyGuard.h, tests/boundary/test_voice_frequency_guards.cpp, branch fix/issue-5-zero-freq-guard", locator: "PR #14", accessed: "2026-11-01", tier: 3 }
confidence: verified
derived_from: []
discovered_by: implementer
run: gen-20261101T-issue-remediation
recorded_at: 2026-11-01T20:00:00Z
---
Contract to lock in tests: on invalid input the voice enters a defined silent state (no NaN/inf reaches the DAC), and the next valid note fully reinitializes the voice state. Mirror the render loop's silent-state check in the test simulation, or the test itself can segfault on inf delay lengths. Source: issue #5 remediation, boundary tests U-B1..U-B4 and integration test I-B1. Pushed to qualitycoding/knowledge @ 3e8f494.
