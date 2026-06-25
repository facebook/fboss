# Trunk / LAG coverage pack (DRAFT — not yet validated)

Status: drafted from a report-mode run over `AgentEcmpTrunkTests.cpp`; NOT validated, NOT --apply-eligible. Needs human review + an eval corpus + a spot-check before --apply (see `../contributing.md`).

This pack is distinct from `ecmp.md`. The ECMP pack is about hash distribution; this pack is about **LAG member lifecycle and its interaction with the ECMP set** (min-links, member-down code paths, member-transition warmboot sequences). Route trunk-lifecycle files (e.g. `AgentEcmpTrunkTests`) to this pack; route trunk *load-balancing* files (e.g. `AgentTrunkLoadBalancerTests`) to `ecmp.md`, which owns the two-stage hash concerns.

## Extra fingerprint fields

Capture under `coveragePack["trunk"]`:

- **`minLinks`** — the aggregate-port min-links threshold (1 vs ≥2). This changes the EXPECTED polarity of the ECMP-set reaction to a member going down (with min-links=1 a single member-down must NOT shrink the ECMP set; with ≥2 it does).
- **`memberDownPath`** — how a member is taken down: `lacp` (`disableTrunkPort`) vs `physical` (`bringDownPort`). These are distinct HW/code paths to the same member-down outcome (and differ in neighbor-purge behavior).
- **`ecmpSizeReaction`** — what is asserted about `getEcmpSizeInHw` / `verifyAggregatePortMemberCount` as members transition (no-change / decrement-by-N / restore).
- **`memberTransitionSequence`** — the ordered member up/down/resolve sequence, especially across a warmboot (e.g. down-then-up-before-reinit, up-down-up, unresolve-then-reresolve). Pairs with the universal `warmbootShape` per-stage object.
- **`aggGeometry`** — number of aggregate ports × members-per-agg (e.g. 4×3, 4×2). Relevant when a routed/scaled member set is exercised.

## Extra gate predicates

A candidate trunk/LAG test is subsumed only if, in addition to the 6 universal gates passing:

- **(a) Min-links parity.** `minLinks` must match. A min-links=1 test (member-down must not shrink ECMP) is never subsumed by a min-links≥2 test (member-down does shrink) — opposite expected outcomes.
- **(b) Member-down-path parity.** `memberDownPath` must match — `lacp` (`disableTrunkPort`) and `physical` (`bringDownPort`) are distinct HW paths.
- **(c) Member-transition sequence parity.** Two tests with different `memberTransitionSequence` (or different placement of the transition across warmboot stages — cross-reference universal Gate 3's per-stage `warmbootShape`) are distinct; do not fold.
- **(d) ECMP-reaction parity.** `ecmpSizeReaction` must match (no-change vs decrement vs restore is the asserted HW behavior).

## Worked notes (from the report-mode run)

- `TrunkNotRemovedFromEcmpWithMinLinks` is non-foldable purely on predicate (a): it is the only `minLinks=1` test (member-down expects `numEcmpMembersToExclude=0`); every other test uses `minLinks=2`.
- Tests 3-6 are all 4-arg warmboot but differ in `memberDownPath` (lacp vs physical) and `memberTransitionSequence` / per-stage placement (test 5 = down-then-up before reinit; test 6 = up-down-up with an empty cold `verify`, so coverage is warmboot-only). Predicates (b)+(c) + Gate 3 keep all four distinct — previously this required reading source line-by-line.
- `TrunkMemberRemoveWithRouteTest` is the only test pairing a scaled mixed-prefix route set (10 prefixes at /48,/64,/128) with per-member-disable `verifyAggregatePortMemberCount` decrements — distinct `aggGeometry`/scale + `ecmpSizeReaction` mutation sequence.
