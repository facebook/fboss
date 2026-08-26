# ECMP / load-balancer coverage pack (DRAFT — not yet validated)

Status: drafted from a report-mode run over `AgentTrunkLoadBalancerTests.cpp`; NOT validated, NOT --apply-eligible. Needs human review + an eval corpus + a spot-check before --apply (see `../contributing.md`).

This pack covers hash distribution / load-balancing. For LAG member-lifecycle concerns (min-links, member-down paths, member-transition warmboot), see `trunk.md`.

## Extra fingerprint fields

Capture under `coveragePack["ecmp"]`:

- **`hashConfig`** — per load-balancer stage, the programmed hash: `{stage: ECMP|AGGREGATE_PORT, fields: [...], algorithm, flowLabel?: bool, udf?: bool}`. Full-hash (includes L4 transport ports) vs half-hash (omits them) on each stage is the primary distinguishing axis of the load-balancer matrix, and the SAME APIs/depth/features/warmboot shape otherwise hide it.
- **`aggGeometry`** — number of aggregate ports × members-per-agg (e.g. 4×3, 4×2). The MxN geometry is deliberately varied to expose hash-bit-reuse / polarization (e.g. T39279972); it is not a free parameter.
- **`injectionPath`** — `cpu` vs `frontPanel` (`loopThroughFrontPanel`). Front-panel exercises the ingress-pipeline hash that CPU injection does not; treat as a real coverage axis for LB tests.
- **`evenness`** — what `isLoadBalanced(...)` asserts: which ports/aggregates are checked, equal-weight vs weighted (UCMP), and the deviation tolerance. An equal-weight LB test does not cover a weighted one.
- **`trafficType`** — IP / IP→MPLS / MPLS-SWAP / MPLS-PHP / SRv6, etc. Different forwarding pipelines and label/encap operations.

## Extra gate predicates

A candidate ECMP/LB test is subsumed only if, in addition to the 6 universal gates passing:

- **(a) Hash-config parity.** `hashConfig` must match per stage (same fields + algorithm on ECMP AND on AGGREGATE_PORT). `ECMPFullTrunkHalf` and `ECMPHalfTrunkFull` are NOT equivalent.
- **(b) Two-stage geometry parity.** When a multi-stage (ECMP × trunk) hash is configured, do not fold tests that differ in `aggGeometry` — the MxN interaction is the coverage (polarization-bug exposure).
- **(c) Injection-path parity.** `injectionPath` must match — a CPU-traffic test does not subsume its front-panel sibling (ingress-hash path).
- **(d) Evenness parity.** `evenness` (port/agg set, equal vs weighted, deviation) must match; weighted/UCMP coverage is distinct from equal-cost.
- **(e) Traffic-type parity.** `trafficType` must match (IP vs MPLS-SWAP vs MPLS-PHP vs SRv6 exercise different pipelines; MPLS/SRv6 are also distinct ProductionFeatures — cross-reference Gate 1).

## Worked notes (from the report-mode run)

- The 40-cell matrix in `AgentTrunkLoadBalancerTest`/`AgentMplsTrunkLoadBalancerTest` is non-collapsible: `ECMPFullTrunkHalf`↔`ECMPHalfTrunkFull` differ only by `hashConfig` (predicate a); 4×3 vs 4×2 differ only by `aggGeometry` and is the documented polarization case (predicate b); CPU vs front-panel cells differ only by `injectionPath` (predicate c). Without these fields the universal gates see them as identical.
- `Srv6TrunkEcmpLoadBalance` is the sole owner of `SRV6_ENCAP` here (Gate 1) and the only flow-label trunk-full SRv6 config.
