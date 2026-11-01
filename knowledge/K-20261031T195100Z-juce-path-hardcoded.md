---
id: K-20261031T195100Z-juce-path-hardcoded
statement: microtonal-guitar's CMakeLists.txt hardcodes the JUCE location as a Windows-only absolute path (add_subdirectory("D:/JUCE" JUCE)); the project cannot configure on other machines without a substitution.
status: current
supersedes: []
tags: [subject:microtonal-guitar-build, tech:juce, tech:cmake, phase:implementation]
applies_to_version: repo commit 9644510
source: { url: "https://github.com/qualitycoding/microtonal-guitar/blob/main/CMakeLists.txt", title: "microtonal-guitar CMakeLists.txt", locator: "add_subdirectory line", accessed: "2026-10-31", tier: 1 }
confidence: verified
derived_from: [C-001]
discovered_by: planner
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: 2026-10-31T19:51:00Z
---
The CMake build requires JUCE at `D:/JUCE`; a portable build needs a FetchContent pin or
`JUCE_PATH`-style cache variable plus the `-D` override ordering invariant (pass overrides as
configure arguments, never env vars).
