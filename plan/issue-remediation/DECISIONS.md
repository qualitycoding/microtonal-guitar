# Decisions

- D-001: Use `main` at `504f5a9`, not the obsolete issue-filing baseline `9644510`.
- D-002: Do not reopen or reimplement closed issues solely because this plan exists. Verify their acceptance criteria and open a focused regression issue only if evidence fails.
- D-003: Keep #5 remediation limited to finite-frequency/delay validation, silence on invalid state, and recovery on a later valid note.
- D-004: Preserve all frozen tests. Add or amend tests only through unfreeze, red-verification, and re-freeze with an updated manifest.
- D-005: Do not publish a tag or GitHub Release without G-002. Do not push lessons or knowledge to external stores without G-003.
- D-006: Omit mutation testing because the suite is small and the critical paths are better gated by boundary, concurrency, sanitizer, and operational tests. Reconsider if review finds inadequate fault detection.
- D-007: Do not change the closed #4 lock-free implementation unless verification demonstrates a violated acceptance criterion. Any writer-side real-time concern found by review must be recorded separately rather than silently expanding #5.
