# Environment (Phase 0.1 + 1.4 pins + 1.5 blocked register)

Probed 2026-10-31T19:43Z in the Kimi cloud sandbox (ephemeral class, see below).

## Environment class: `ephemeral` (0.1)

| Probe | Evidence |
|---|---|
| Durable mount | `/mnt/agents` survives across tool calls (working files from earlier calls intact) |
| Temp survival | `/tmp` marker written 19:43Z still readable at next call — but this is intra-session; per the observed table, Kimi cloud is ephemeral and unknowns classify as ephemeral |
| Deadline kills | One tool call during this run died with `context deadline exceeded` (mid `git clone`) — deadline-kill behaviour matches the ephemeral class |
| Observed table | Protocol 0.1: Kimi on cloud = ephemeral |

Consequence: every `[ephemeral]` section applies; durable surface = `/mnt/agents` only.

## Verified pins (existence + spike-run per 1.3.6)

| Tool | Pinned version | Existence check | Command verified by |
|---|---|---|---|
| git | 2.39.5 | `git --version` in sandbox | used throughout the run |
| g++ (Debian) | 12.2.0-14+deb12u1 | `g++ --version` | spike build below |
| Linux kernel | 5.10.134-18.0.12.lifsea8.x86_64 | `uname -a` | n/a |
| cmake | — | **absent** (`which cmake` empty) | n/a |

Spike (R6-style, committed under `research/spikes/`):
`g++ -std=c++17 -Wall -Wextra research/spikes/tuning_engine_spike.cpp TuningEngine.cpp`
→ build clean, zero warnings; run output `SPIKE RESULT: ALL PASS` (6/6 checks: 12-TET octave &
fifth, 24-TET quarter-tone, 31-EDO octave, 5-limit 3/2, invalid-index 0.0).

## JUCE / plugin-build dimension — environment-blocked (1.5 item 6, D-ADD-003)

* The shipped `CMakeLists.txt` hardcodes `add_subdirectory("D:/JUCE" JUCE)` (Windows-only path;
  REVIEW.md G-3) — no portable setup command exists to verify.
* `cmake` is absent from the sandbox; a full JUCE VST3 build exceeds one container lifetime.
* This dimension is recorded **blocked (environment limitation) — never reported as green**.
  CI equivalent: a future full-protocol run defines a CI job (portable JUCE FetchContent pin +
  `cmake --build`) as the identical named job. Substitute local verification: the JUCE-free
  `TuningEngine` g++ spike above.

## Host reachability

* `github.com` (clone, API, raw): reachable.
* `codeload`/raw.githubusercontent.com: reachable (raw file fetches succeeded).
* Knowledge store `qualitycoding/knowledge`: 404 — does not exist (R0 loads nothing; D-ADD-005).

## Ephemeral-surface notes for implementers (1.5 items 1–4)

1. Only `/mnt/agents` is durable; `/tmp` survives intra-session but must not be trusted across
   recycles. This run staged the compiled spike binary to `/tmp` to execute it.
2. The output mount **forbids exec bits** (`chmod +x` is accepted but execution still returns
   Permission denied) — run compiled binaries from a staged local-disk copy, or invoke scripts
   via an interpreter (e.g. `python3 script.py`), never `./script` on the mount.
3. No background process outlives its tool call — keep all work foreground and checkpointed.
4. Pass dependency overrides as `-D` configure args, never env vars (CMake ignores the env
   overrides); cache downloaded archives on the mount.
