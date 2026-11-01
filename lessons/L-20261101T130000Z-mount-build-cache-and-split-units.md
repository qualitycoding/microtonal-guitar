# L-20261101T130000Z-mount-build-cache-and-split-units

- **Trigger:** building the JUCE 9 integration harness took six sandbox recycles; each retry
  taught something new about why a naive unity build was incomplete.
- **Mistake:** assumed `juce_<module>.cpp` was the whole module, and assumed the FUSE mount could
  hold build outputs.
- **Correction:** (1) JUCE 9 splits large modules into `juce_gui_basics_2..5.cpp`; widget code is
  NOT in the primary unity file. (2) `juce_audio_processors` delegates its core to
  `juce_audio_processors_headless` (AudioProcessor, parameters). (3) Extra root-level units exist
  per module (`juce_core_CompilationTime.cpp`, `juce_graphics_lunasvg.c`, harfbuzz, sheenbidi,
  libjpg/libpng/libwebp amalgamations, SIMD webp units needing `-msse4.1`/`-mavx2`). (4) The
  assembler cannot write `.o` directly to the FUSE mount (BFD assertion elf.c:3108) — compile to
  /tmp and copy. (5) `.c` amalgamations must be compiled with gcc, not g++.
- **Prevention rule:** enumerate units with `ls modules/<m>/*.{c,cpp}` and the `_<digit>` split
  pattern before building; compile C units with gcc; cache objects on the mount via tmp-then-copy.
- **Detection check:** after linking, `nm` the module objects for a known symbol
  (e.g. `Label::setText` must be defined, not U, in gui_basics).
- **Tags:** juce9, build, ephemeral, mount
