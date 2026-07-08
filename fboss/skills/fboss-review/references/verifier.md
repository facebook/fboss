# Verifier: Dedup, Confidence Calibration, False Positive Filtering

## Deduplication

- Same file:line reported by multiple reviewers = single finding
- Same underlying issue across nearby lines = single finding (use the most specific location)
- When merging, keep the highest severity and combine reviewer attributions

## Confidence Calibration

Starting confidence is the reviewer's assigned value (0.0-1.0). Adjust:

- **+0.2** if 2 or more reviewers independently flagged the same issue
- **-0.2** if the flagged pattern is a common, established pattern in the surrounding codebase (check 5+ nearby files)
- **+0.1** if the pattern matches a HIGH confidence item from fboss-code-standards
- Cap final confidence at 1.0

## Anti-Downgrade Rules

Do NOT reduce severity below MEDIUM for these, even if surrounding code follows the same pattern:
- **Hardcoded magic numbers when enums/constants exist**

## False Positive Filtering

Remove findings that match any of these:
- File has `@generated` tag (auto-generated code, do not review)
- Issue exists in code not touched by this diff (pre-existing, out of scope)
- Test-only finding with severity LOW (not worth flagging)
- Style-only preference with no functional impact

## Threshold

Only report findings with final confidence >= 0.7.

## Output Format

Present verified findings as a structured table:

```
| File:Line | Reviewer(s) | Severity | Issue | Confidence |
|-----------|-------------|----------|-------|------------|
```

Sort by severity (HIGH first), then by confidence (highest first).
