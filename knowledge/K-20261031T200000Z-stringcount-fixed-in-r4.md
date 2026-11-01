---
id: K-20261031T200000Z-stringcount-fixed-in-r4
statement: The inert stringCount parameter (currentNumStrings never stored) is fixed on the r4 generation branch — updateGuitarState() now calls currentNumStrings.store(newState.numStrings); the fix is present from branch gen-20260919T224942Z-fret-note-names-r4 onward, still absent from main.
status: current
supersedes: [K-20261031T195200Z-stringcount-param-inert]
tags: [subject:microtonal-guitar, tech:juce, tech:c++, phase:implementation]
applies_to_version: branch gen-20260919T224942Z-fret-note-names-r4 and descendants
source: { url: "https://github.com/qualitycoding/microtonal-guitar/blob/gen-20260919T224942Z-fret-note-names-r4/PluginProcessor.cpp", title: "r4 PluginProcessor.cpp", locator: "line 129", accessed: "2026-10-31", tier: 1 }
confidence: verified
derived_from: [C-003]
discovered_by: implementer
run: gen-20261031T194500Z-protocol-v36-addendum
recorded_at: 2026-10-31T20:00:00Z
---
Post-merge of r4 into the addendum branch, the fix is live on the merged line. main remains
unfixed. The D:/JUCE hardcode (REVIEW.md G-3) is NOT fixed by r4.
