---
id: K-20261031T195200Z-stringcount-param-inert
statement: The stringCount APVTS parameter never reaches the string-allocation logic — MicroGuitarProcessor::currentNumStrings is initialized to 6 and has no store sites, so processBlock always allocates 6 strings and getNumStrings() always reports 6 regardless of the 4–12 slider.
status: current
supersedes: []
tags: [subject:microtonal-guitar, tech:juce, tech:c++, phase:verification]
applies_to_version: repo commit 9644510
source: { url: "https://github.com/qualitycoding/microtonal-guitar", title: "microtonal-guitar source grep", locator: "PluginProcessor.{h,cpp}", accessed: "2026-10-31", tier: 1 }
confidence: verified
derived_from: [C-002]
discovered_by: planner
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: 2026-10-31T19:52:00Z
---
Static evidence committed at research/spikes/currentnumstrings_static.output.txt (three load
sites, zero store sites). Fix: updateGuitarState() should currentNumStrings.store(newState.numStrings).
