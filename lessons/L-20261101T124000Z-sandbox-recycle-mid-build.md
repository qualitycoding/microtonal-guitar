# L-20261101T124000Z-sandbox-recycle-mid-build

- **Trigger:** a long JUCE build exceeded the sandbox lifetime; the sandbox was recycled mid-build and `/tmp` (build dir, staged JUCE clone) was wiped. The next tool call returned `no sandboxes found with the provided IDs`.
- **Mistake:** the JUCE checkout and build tree lived only in `/tmp`; nothing pushable had been committed since the previous green chunk, so the interrupted work had no durable state.
- **Correction:** re-stage from the persistent mount: `git clone JUCE` into `/mnt/agents/juce-cache` once; rebuild in `/tmp` per sandbox; keep every green chunk committed and pushed immediately (Rule 6) so a recycled sandbox loses at most the current chunk.
- **Prevention rule:** caches of expensive third-party checkouts live on the durable mount (`/mnt/agents/<name>-cache`), never only in `/tmp`; after any recycle, re-run the S-ADD-003 restage check before building.
- **Detection check:** before a long build, confirm the cache path exists on the mount (`test -d /mnt/agents/juce-cache/modules`); after a recycle, re-run the clone if missing.
- **Tags:** environment, ephemeral, build, restaging
