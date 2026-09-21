# Contributing Patterns

## Where to Add Your Pattern

```
Is your pattern FBOSS-specific?
+-- YES -> Which area?
|   +-- Agent/SwSwitch/HwSwitch    -> agent-patterns.md
|   +-- SAI/SDK                     -> sai-sdk-patterns.md
|   +-- FSDB/thrift_cow            -> thrift-cow-fsdb-patterns.md
|   +-- Platform/Config            -> platform-config-patterns.md
|   +-- Testing                    -> testing-patterns.md
|   +-- Cross-cutting design judgment (not a single Check/Why rule)
|       -> fboss-review/references/fboss-specific-reviewers.md (Architect reviewer)
+-- NO -> Is it a coding convention the team wants enforced?
    +-- YES -> general-patterns.md
    +-- NO  -> Probably doesn't belong here
```

## Pattern Template

Every pattern must follow this format:

```markdown
### N. Pattern Name
**Check**: What specifically to look for
**Why**: Consequence of violation
**Confidence**: HIGH (always a bug) or MEDIUM (context-dependent)
```

## Guidelines

- **Evidence-based only**: Patterns must come from real bugs, SEVs, or repeated review feedback. No hypothetical issues.
- **False positive test**: Before adding, check 3-5 recent diffs to confirm the pattern would not produce false positives.
- **HIGH confidence**: Use only when violation is always a bug, regardless of context.
- **MEDIUM confidence**: Use when violation is usually a problem but some exceptions exist.
- **No style preferences**: Style-only preferences (brace placement, blank lines) do not belong here.
- **Diff prefix**: When submitting pattern changes, use `[fboss][claude]` prefix in the diff title.
