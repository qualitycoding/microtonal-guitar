# Environment

- Repository: `qualitycoding/microtonal-guitar`
- Baseline: `504f5a93eeb081ac78776ef74b91142fe6307ef4`
- Implementation branch: `fix/issue-5-zero-freq-guard` if it is based on the current baseline; otherwise recreate it from `main` and record the substitution in `DEVIATIONS.md`.
- Language: C++; JUCE plugin; CMake minimum declared by the repository is 3.22.
- JUCE: use the repository-pinned `MICROGUITAR_JUCE_TAG`; do not silently change it.
- CI: versioned Windows, Linux, and macOS runners already declared in `.github/workflows/build.yml`; use `fail-fast: false`.
- Required local/CI capabilities: Git, CMake, Ninja, a supported C++ compiler, CTest, Python 3, and network access to GitHub and the JUCE repository.
- Required manual environment: Windows, Ableton Live, Release VST3, and an operator.
- Every command substitution, version change, timeout, or configuration change must be recorded in `DEVIATIONS.md` when made.
