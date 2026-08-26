---
name: hw-test-dedup
description: Find and remove redundant FBOSS agent_hw_tests without losing coverage. Use when asked to reduce/combine/dedup HW tests, retire non-dataplane (set/get) tests where dataplane tests exist, or cut HW test runtime while keeping coverage. Report-first; applying edits is a separate opt-in step.
metadata:
  oncalls: fboss_agent
  strict: true
  apply_to_path: 'fbcode/fboss/agent/test/agent_hw_tests/.*\.cpp$'
  apply_to_user_prompt: '(?i)(dedup|redundant|fold|combine|reduce).*(hw[ _]?test|agent_hw_test)'
---

# HW Test De-duplication

Given a set of FBOSS `agent_hw_tests`, find test cases whose hardware coverage is fully contained in other tests, and recommend removing them — **fewer tests, identical coverage**. Default output is a report; turning recommendations into code edits is a separate, confirmation-gated step.

## IMPORTANT: report-first, never auto-post

- Default run produces a **report only**. It never edits tests or creates diffs unless `--apply` is passed AND the user confirms.
- It **never** submits/publishes to Phabricator on its own.
- When a removal cannot be proven safe, the test is **kept** and the reason is written down. Bias is always toward keeping coverage.

## When to use

- "dedup / combine / reduce these HW tests", "retire set/get tests where we have dataplane tests", "cut HW test count without losing coverage".
- Scope: `fbcode/fboss/agent/test/agent_hw_tests/` (this version). Other test trees are out of scope.

## Invocation

| Form | Meaning |
|---|---|
| `/hw-test-dedup <file1> <file2> ...` | Analyze an explicit set of test files. |
| `/hw-test-dedup <dir>` | Discover + cluster the test files under a dir, then analyze per cluster. |
| `--apply` | After analysis, apply the folds/removals as **draft** diffs (gated; see Apply rules). |
| `--deep-verify` | Opt-in empirical SAI call-trace check for top candidates (needs build/run). |
| `--self-test` | Run against the pinned eval corpus and diff verdicts vs `references/eval/`. |
| `--gap-report <D1,D2,...>` | Compare this skill's recommendations to already-landed reduction diffs; emit a gap report. |
| `--resume` | Continue from the last completed phase checkpoint. |

## Steps the agent follows

The orchestration is implemented in `scripts/dedup-workflow.js` (run via the Workflow tool). Follow these steps:

1. **Resolve inputs.** Expand the file list (or discover files under the dir). Confirm the cluster grouping with the user when in `<dir>` mode.
2. **Pre-filter (deterministic, no LLM).** Run `scripts/prefilter.js` to cluster by fixture base + feature area and prune pairs that obviously can't overlap. Pruned pairs are recorded for the report.
3. **Profile each test (fingerprint).** Per the **Fingerprinter** persona (`references/personas.md`) and `references/fingerprint-schema.md`, emit a coverage profile per test. Resolve helper code transitively. Pass the roster **by file reference**, not inlined.
4. **Count-check completeness (deterministic).** Run `scripts/roster-count.js`; the fingerprint count per file must equal the `TEST_F/TEST_P/TYPED_TEST` count. A mismatch hard-fails the file.
5. **Detect overlaps + apply gates.** Per the **Subset-detector** persona over pre-filtered pairs, under the 6 universal gates (`references/universal-gates.md`) plus the active coverage pack (`references/coverage-packs/<area>.md`).
6. **Second opinions (adversarial).** Per the **Skeptic** persona: 1 lens in report mode, 3 diverse lenses (unanimous-safe) in `--apply` mode. Refute-by-default; ≥1 reads raw source.
7. **Reconcile (deterministic).** Build the subset→superset graph; keep a maximal antichain; never remove a node whose only superset is itself removed; last-feature-owner is never removable.
8. **Merge-plan.** Per the **Merge-planner** persona, list the unique assertions to port into each survivor first; portability check.
9. **Report** (and, with `--apply`, perform merge → delete → `arc f`/`arc lint` → review pass → one commit per file → **draft** diffs).

## Hard safety rules (enforced in code by the workflow)

- A test that is the **last owner** of any `ProductionFeature` is never removable.
- A removal requires the surviving test(s) to satisfy **all** universal gates (`references/universal-gates.md`).
- `--apply` is allowed for an area only once that area has (a) a validated coverage pack, (b) its own eval corpus with a passing `--self-test`, and (c) a human spot-check of a pilot report. ACL is the first such validated area; every other area is report-only until it meets the bar.

## Reference material

| File | What |
|---|---|
| `references/personas.md` | The 6 agent personas + their output schemas. |
| `references/universal-gates.md` | The 6 universal safety gates. |
| `references/fingerprint-schema.md` | The per-test coverage-profile schema. |
| `references/warmboot-semantics.md` | `verifyAcrossWarmBoots` behavior across the 4 base classes. |
| `references/coverage-packs/*.md` | Per-area extra checks (acl validated; ecmp/mirror/trunk/srv6 drafted, report-only; qos/copp stubs). |
| `references/eval/` | Ground-truth corpora + how `--self-test` / `--gap-report` use them. |
| `references/contributing.md` | How to add a coverage pack / gate / eval corpus. |
| `references/design.md` | The design of record. |
