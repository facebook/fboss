# Design of `hw-test-dedup`

This is the design of record, written to be read on its own. It assumes no prior knowledge of the skill.

## The goal

FBOSS ships a large suite of hardware (HW) tests. Over time the suite accumulates tests that exercise the same behavior on the same hardware in the same way — true duplicates and near-duplicates. Each redundant test costs lab time, capacity, and signal-to-noise on every run.

The goal of this skill is narrow and strict: **end up with fewer HW tests while keeping exactly the same coverage.** Removing a test is acceptable only when another test (or set of tests) already exercises everything that test exercised. If there is any doubt, the test stays. The skill optimizes for fewer tests *subject to* a hard no-coverage-loss constraint — never the other way around.

## End-to-end pipeline

The skill runs as an ordered pipeline. Each stage narrows or enriches the working set, and the safety-critical reasoning is concentrated in the middle stages.

1. **Resolve inputs.** Determine the exact set of tests under consideration — from the targets, areas, or filters the caller provided — and resolve them to concrete, addressable test identities.

2. **Group / cluster.** Partition the resolved tests into clusters of plausibly-related tests (by area, by what they touch, by shape). Redundancy is only ever evaluated *within* a cluster, which keeps later comparisons tractable and meaningful.

3. **Deterministic pre-filter.** Within each cluster, cheaply discard pairs that obviously cannot be redundant, before any expensive analysis. This is a pure efficiency step. It can cost recall (a real fold might be pruned) but never costs safety (a pruned pair just means a test is kept). Every pruned pair is recorded so the recall trade-off is auditable.

4. **Profile each test (fingerprint).** For every surviving test, build a structured *fingerprint*: the set of attributes that capture what the test actually covers and how — what it exercises, under what conditions, on what hardware, in what run mode. Two tests can only be compared through their fingerprints.

5. **Count-check completeness.** Before trusting any comparison, verify that the fingerprints are complete — that the expected attributes were actually captured for the tests in play. An incomplete fingerprint must not be allowed to make two tests *look* identical merely because data is missing.

6. **Detect overlaps under the gates.** For each candidate pair, decide whether one test's coverage is truly subsumed by the other. A pair may be called redundant only if it passes **six universal gates** (safety predicates that apply to every area) plus any predicates from the area's **coverage pack** (area-specific safety rules layered on top). Failing any gate keeps both tests.

7. **Adversarial second opinions.** Candidate redundancies are re-examined by independent adversarial passes that actively try to find a reason the pair is *not* redundant. A pair is only carried forward if these passes are **unanimous that folding it is safe.** A single dissent demotes it to "keep" or "uncertain."

8. **Deterministic reconcile.** Turn the surviving pairwise redundancies into a consistent global decision, deterministically. This computes a **maximal antichain** — the largest set of removals that is mutually consistent (you never remove two tests that were each other's justification) — under **last-owner protection**, which forbids removing the last remaining test that owns a given piece of coverage.

9. **Merge plan.** Produce a concrete plan: which tests to remove, and for each, which surviving test(s) carry the coverage that justifies the removal.

10. **Report.** Emit a human-readable report: the proposed removals with their justifications, the pairs that were kept and why, the pairs the pre-filter pruned, and anything flagged uncertain.

`--apply` is **opt-in** and even then produces **draft diffs only** — it never lands anything and never publishes.

## Deterministic vs. agent split

The pipeline deliberately separates two kinds of work:

- **Deterministic stages** — input resolution, the pre-filter, the count-check, and the reconcile (maximal antichain + last-owner protection). These are reproducible and order-independent: the same inputs yield the same removals every time. The final go/no-go on *what gets removed* is deterministic.
- **Agent stages** — fingerprint profiling, overlap detection under the gates, and the adversarial second opinions. These require judgment about what a test really covers and whether two tests are genuinely equivalent.

The agent stages can only ever *propose* a fold; the deterministic reconcile decides which proposals survive into the plan. Judgment proposes, determinism disposes.

## Points of interest

These are the cases where naive "are these two tests the same?" reasoning goes wrong, and how the design handles each.

- **Warmboot callback semantics.** Warmboot tests run code across a restart boundary via callbacks (before/after the warmboot). Two warmboot tests can look identical by name and surface yet exercise different callback phases. Fingerprints capture the warmboot callback structure so tests that differ only across the warmboot boundary are not mistaken for duplicates.

- **Last-feature-owner.** A test may be the *only* remaining test exercising some feature, even if it overlaps heavily with another test on everything else. Last-owner protection in the reconcile stage forbids removing such a test, so the feature never loses its sole owner.

- **ASIC / SDK gating.** Tests are often gated to run only on particular ASICs or SDK versions. Two tests that appear equivalent may actually cover disjoint ASIC/SDK combinations. The gating is part of the fingerprint and a universal gate, so a pair is not folded unless their ASIC/SDK coverage genuinely coincides.

- **Single-vs-multi run mode.** A test can behave differently when run standalone versus as part of a multi-test run (shared setup, ordering, state). Run mode is part of the fingerprint, and tests covering different run modes are not treated as redundant.

- **Atomic-vs-incremental delta path.** Configuration can be applied as a single atomic change or as an incremental series of deltas, and these exercise different code paths. The delta path is fingerprinted so a test covering the atomic path is not folded into one covering the incremental path (or vice versa).

## Outputs and guardrails

The skill's behavior is constrained by a small set of non-negotiable guardrails:

- **Never remove a last owner.** No removal may leave any piece of coverage without a test.
- **Never auto-publish.** `--apply` produces draft diffs only; nothing is landed or published automatically.
- **Report-first.** The default output is a report. Removal is a separate, explicit opt-in.
- **When in doubt, keep.** Any unresolved uncertainty — incomplete fingerprint, gate failure, adversarial dissent — resolves to keeping the test.

## Validation

Confidence in the skill rests on evidence, and the design is honest about how much evidence exists.

- The skill ships with an **ACL self-test**, a pinned corpus for a single area. This is **N = 1**: it is *necessary* (a regression here means something broke) but *not sufficient* (passing it does not prove the approach generalizes).
- Before `--apply` is allowed to spread to more areas, the bar is **at least two validated areas plus a human spot-check** — two independent corpora passing, and a human confirming a pilot report looks right.
- A **second independent corpus** can be sourced from **already-landed reduction diffs**: past, human-reviewed test removals are a ground-truth set of "these really were redundant." The `--gap-report` mode replays the skill against those landed reductions to check whether it would have reached the same conclusions, providing validation evidence that is independent of the hand-built ACL corpus.
