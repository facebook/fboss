# Contributing to `hw-test-dedup`

This document explains how to extend the skill safely. The unifying principle is the same one that governs the skill itself: a change may improve how many redundant tests we *find*, but it must never weaken the guarantee that nothing covered today stops being covered tomorrow.

## Adding a coverage pack for a new area

A coverage pack teaches the skill how to reason about redundancy in one specific area of the HW-test surface (for example, a particular ASIC family, a feature subsystem, or a class of warmboot tests). To add one:

1. Create `references/coverage-packs/<area>.md`.
2. In that file, specify two things:
   - **(a) Extra fingerprint fields** — the additional attributes that must be captured when profiling a test in this area so two tests can be compared honestly. These are area-specific signals that the universal fingerprint does not already capture.
   - **(b) Extra gate predicates** — additional safety predicates, layered on top of the six universal gates, that two tests must both satisfy before they can be called redundant in this area.
3. Wire the area's name into clustering so that tests belonging to this area are grouped together and the pack is consulted when those clusters are profiled and compared.

The discoverer persona may draft a new coverage pack, but **a human must review and approve it before its first use.** An unreviewed pack can silently encode an incorrect assumption about what "same coverage" means, and that error would propagate into every disposition for the area.

## Adding or changing a universal gate

Universal gates apply to every area and are the core safety net.

1. Edit `references/universal-gates.md` to add or modify the gate.
2. Add an **eval case** that proves the gate's effect — a concrete pair of tests where the gate changes the disposition (e.g., a pair that would be called redundant without the gate but is correctly kept once the gate is present). A gate change without a demonstrating eval case is not complete.

## Adding an eval corpus

An eval corpus is the skill's regression harness: a set of test pairs with known, human-blessed dispositions.

1. Add a pinned **expected-dispositions JSON** file under `references/eval/`. Each entry pins a pair (or group) of tests to its expected disposition (redundant / keep / uncertain).
2. Reference the new corpus from `--self-test` so it runs as part of the self-test suite.

Pinning the expected dispositions is what makes the corpus a regression test: if a later change to fingerprints, gates, or reconciliation flips one of these dispositions, the self-test fails and the regression is caught before it reaches a report.

## Hard rule for enabling `--apply` on a new area

`--apply` (which produces draft merge diffs) may be enabled for an area **only after all three of the following exist**:

1. A **validated coverage pack** for the area (drafted, then human-reviewed and approved).
2. The area's **own eval corpus** — at least two to three known-redundant pairs, each with an expected disposition.
3. A **human spot-check of a pilot report** produced by the skill for that area.

Until all three are in place, the area is **report-only**: the skill may surface candidate redundancies for that area, but it must not offer `--apply` for it.

## Tuning the pre-filter

The deterministic pre-filter prunes pairs before the expensive profiling and comparison stages. When tuning it, keep this trade-off clear:

- Pruning trades away **recall** — an aggressive pre-filter may drop a pair that was in fact a foldable redundancy, so that fold is simply missed.
- Pruning never trades away **safety** — a missed fold means a test that *could* have been removed is kept, which loses no coverage.

To keep recall loss auditable, **every pruned pair is surfaced in the report.** That way a reviewer can see exactly which candidates the pre-filter discarded and judge whether the filter was too aggressive, rather than having folds disappear silently.
