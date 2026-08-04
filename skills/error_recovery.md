---
name: Error Recovery
description: Instructions for handling tool execution failures and self-correction.
keywords: error, fail, recovery, fix, retry, exception, fallback, debug
---

# Error Recovery Skill

When a tool returns an error message or fails:
1. Do not repeat the exact same tool call with identical arguments.
2. Analyze the root cause from the error text (missing file, syntax error, denied permission).
3. Attempt an alternative approach or adjust parameters (e.g. check path, rewrite input).
4. If a tool remains unavailable, inform the user clearly with the exact failure details.
