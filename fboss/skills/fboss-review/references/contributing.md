# Contributing to FBOSS Review

## Adding a New Reviewer

1. Decide if the reviewer is generic (applies to all code) or FBOSS-specific (applies to FBOSS domain).
2. Generic reviewers: add persona to `generic-reviewers.md` following the existing format.
3. FBOSS-specific reviewers: add patterns to the appropriate file in `../fboss-code-standards/references/`. The reviewer dispatching logic in SKILL.md determines which pattern file each reviewer loads.

## Modifying Reviewer Personas

- Keep focus areas specific and non-overlapping with other reviewers.
- Include severity guidance so the reviewer calibrates findings consistently.
- Test changes by running `/fboss-review` on 2-3 recent diffs and checking for false positives.

## Adjusting Verification

- Modify `verifier.md` to change confidence calibration rules or the reporting threshold.
- The default threshold of 0.7 balances signal vs noise. Lower only if reviewers consistently under-rate real issues.

## Pattern Files vs Reviewer Personas

- **Pattern files** (in `fboss-code-standards/references/`): specific, checkable rules. Team-extensible.
- **Reviewer personas** (in `generic-reviewers.md`): broad focus areas and severity guidance. Stable, rarely edited.
- New domain-specific rules go in pattern files. New review dimensions go as reviewer personas.

## Adding Architectural Concerns

For cross-cutting design concerns that don't fit a single pattern file:
1. Add the principle to the Architect reviewer (#11) persona in `fboss-specific-reviewers.md`.
2. Include: principle name, what to look for, example from a real diff with quote, and severity.
3. Keep principles at MEDIUM severity — architectural feedback suggests improvements, not bugs.
4. Test by running `/fboss-review` on 2-3 diffs where the concern would apply.
