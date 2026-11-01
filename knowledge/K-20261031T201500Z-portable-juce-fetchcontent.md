---
id: K-20261031T201500Z-portable-juce-fetchcontent
statement: The D:/JUCE hardcode is replaced by FetchContent pinning JUCE tag 9.0.4 (existence-verified 2026-10-31); a local/air-gapped JUCE is supplied via -DFETCHCONTENT_SOURCE_DIR_JUCE (a -D configure argument, never an env var); GitHub Actions (.github/workflows/build.yml) builds VST3+Standalone on windows-2022/ubuntu-22.04/macos-14 with apt-installed ALSA/X11/freetype/webkit/curl dev packages and uploads artifacts with existence post-conditions (if-no-files-found: error).
status: current
supersedes: [K-20261031T195100Z-juce-path-hardcoded]
tags: [subject:microtonal-guitar-build, tech:juce, tech:cmake, tech:github-actions, phase:release]
applies_to_version: branch gen-20261031T194500Z-protocol-v36-addendum from commit "build: portable JUCE" onward
source: { url: "https://github.com/qualitycoding/microtonal-guitar/blob/gen-20261031T194500Z-protocol-v36-addendum/CMakeLists.txt", title: "portable CMakeLists.txt", locator: "FetchContent_Declare block", accessed: "2026-10-31", tier: 1 }
confidence: verified
derived_from: [C-004]
discovered_by: implementer
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: "2026-10-31T20:15:00Z"
---
Local sandbox verification: unit tests configure/build/pass under cmake 4.4.5 + ninja; plugin
configure proceeds through FetchContent, juceaide setup and juce_add_plugin, stopping only at
JUCE's pkg-config ALSA check (no dev headers, no root) — the compile dimension completes in CI.
