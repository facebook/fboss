---
name: fboss-review
description: Comprehensive FBOSS code review with 11 parallel reviewers (5 generic + 6 FBOSS-specific). Covers reliability, engineering, code quality, silent failures, agent architecture, SAI/SDK, FSDB/thrift_cow, platform/config, testing, and cross-cutting design. Self-contained alternative to /review-diff for FBOSS diffs. Findings shown to user only, never auto-posted.
user-invocable: true
---

# FBOSS Review

## Overview

Multi-reviewer code review for FBOSS diffs. Subsumes general review (`/review-diff` functionality) plus FBOSS-specific domain expertise.

## Scope

Currently `fboss/` only. TODO: extend to `configerator/source/neteng/fboss`, `neteng/netcastle`, `neteng/fboss`.

## IMPORTANT: No Auto-Posting

All findings are shown to the user only. Never post to Phabricator automatically.

## Review Workflow

### Step 1: Identify Changes

- Run `sl status` and `sl diff` to get changed files
- If user specifies a diff number, fetch diff details via `mcp__plugin_meta_mux__get_phabricator_diff_details`

### Step 2: Determine Applicable FBOSS Reviewers

- Files in `agent/`, `SwSwitch`, `HwSwitch` -> Agent Reviewer (#6)
- Files in `fsdb/`, `thrift_cow/` -> FSDB/thrift_cow Reviewer (#7)
- Files in `platform/`, config, sensor/fan -> Platform Reviewer (#8)
- Files in `sai/`, `hw/sai/`, SDK -> SAI/SDK Reviewer (#9)
- Test files or coverage-affecting changes -> Testing Reviewer (#10)
- Changes spanning SwSwitch + HwSwitch, new SwitchState fields, new counters, BUCK target changes, new client infrastructure, or 3+ directory scope -> Architect Reviewer (#11)

### Step 3: Dispatch Reviewers in Parallel

Use Agent tool (sonnet model) to dispatch all applicable reviewers simultaneously:

- **Generic reviewers 1-5**: ALWAYS dispatch. Use personas from `references/generic-reviewers.md`.
- **FBOSS reviewers 6-10**: Dispatch ONLY if their area is touched.
- Each reviewer receives: full diff content + their persona instructions.
- FBOSS reviewers additionally load patterns from `../fboss-code-standards/references/<area>-patterns.md`.

| # | Reviewer | Focus | Patterns File |
|---|----------|-------|---------------|
| 1 | Reliability | Error handling, RAII, logging, timeout/retry, graceful degradation | - |
| 2 | Engineering / Performance | Algorithmic complexity, unnecessary copies, lock contention, modern C++ | - |
| 3 | Code Quality | Readability, modularity, duplication, API design | `../fboss-code-standards/references/general-patterns.md` |
| 4 | Summary & Test Plan | Title accuracy, summary completeness, test plan adequacy, diff coherence | - |
| 5 | Silent Failure Finder | Logic errors, lossy conversions, race conditions, silent data loss | - |
| 6 | Agent Architecture | Mono/multi-switch, state management, warmboot, HwSwitch/SwSwitch boundary | `../fboss-code-standards/references/agent-patterns.md` |
| 7 | FSDB & thrift_cow | State/Stats duality, COW node safety, subscriptions, build time | `../fboss-code-standards/references/thrift-cow-fsdb-patterns.md` |
| 8 | Platform & Config | Platform services, JSON configs, startup order, OSS sync | `../fboss-code-standards/references/platform-config-patterns.md` |
| 9 | SAI/SDK Integration | SAI API usage, object lifecycle, vendor SDK compat, attributes | `../fboss-code-standards/references/sai-sdk-patterns.md` |
| 10 | Testing Standards | Coverage, naming, fixtures, NSDB impact, HW test patterns | `../fboss-code-standards/references/testing-patterns.md` |
| 11 | FBOSS Architect | Cross-cutting design: layering, state derivability, abstraction, code org, infra reuse | `references/fboss-specific-reviewers.md` |

### Step 4: Verification

Dispatch a single verifier agent (loads `references/verifier.md`) to:
- Deduplicate findings across reviewers
- Calibrate confidence scores
- Filter false positives
- Threshold: only report findings with confidence >= 0.7

### Step 5: Present Findings

Output a structured report:

```
| File:Line | Reviewer | Severity | Issue | Confidence |
|-----------|----------|----------|-------|------------|
```

Never post findings to Phabricator.

## When to Load References

| Topic | Action |
|-------|--------|
| Generic reviewer personas | Read `references/generic-reviewers.md` |
| Verification/dedup logic | Read `references/verifier.md` |
| FBOSS-specific reviewer personas | Read `references/fboss-specific-reviewers.md` |
| Adding/modifying reviewers | Read `references/contributing.md` |
