# Mirror coverage pack (DRAFT — not yet validated)

Status: drafted from a report-mode run over `AgentHwMirrorTests.cpp` + `AgentMirroringTests.cpp`; NOT validated, NOT --apply-eligible. Needs human review + an eval corpus + a spot-check before --apply (see `../contributing.md`).

## Extra fingerprint fields

Capture under `coveragePack["mirror"]`:

- **`mirrorType`** — `span` (local egress-port destination) / `erspan` (GRE tunnel) / `sflow`. These program different HW (tunnel vs no tunnel).
- **`direction`** — `ingress` / `egress` (from `isIngress()` / the `kIngress*` vs `kEgress*` mirror constants). Egress mirror attach is a distinct HW path.
- **`sampleDest`** — `MIRROR` vs `CPU` (`SampleDestination`). Flips mid-test in some sflow teardown cases; distinguishes otherwise near-identical tests.
- **`truncationKind`** — distinguishes the three truncation semantics: config-truncate flag on SPAN/ERSPAN; the `MIRROR_PACKET_TRUNCATION` vs `EGRESS_MIRROR_PACKET_TRUNCATION` features (separate ProductionFeatures); and the dataplane byte-count assertion (`outBytes <= 1500`). A single boolean is insufficient.
- **`sampledPortScale`** — one sampled port vs all ports (mirror's specialization of the universal Gate 6 scale axis).
- **`portBindings`** — per-port mirror-binding vector with the flag bitmask `{ingress=1, egress=2, sflow=4}` and the resolved/unresolved expectation, as verified by `verifyPortMirrorDestination(port, flags, destId)`. None of the core schema fields capture this.
- **`resolutionSequence`** — mirror-resolution lifecycle transitions (unresolved ↔ resolved ↔ re-resolved), distinct from config mutation.
- **`resourceStats`** — exact mirror resource counters asserted (`mirrors_used`/`span`/`erspan`/`sflow`); these are resource-count stats, not traffic counters, so `counterTypes` (PACKETS/BYTES) is the wrong vocabulary.
- **`verificationFamily`** — `hwState` (HW-introspection: `verifyResolvedMirror`/`verifyPortMirrorDestination`, `trafficMode=none`) vs `dataplane` (`sendPackets` + port stats). For mirror these families live in different files and call disjoint APIs.

## Extra gate predicates

A candidate mirror test is subsumed only if, in addition to the 6 universal gates passing:

- **(a) Mirror-type containment.** `superset.mirrorType ⊇ subset.mirrorType` (blocks span↔erspan↔sflow folds).
- **(b) Direction containment.** `superset.direction ⊇ subset.direction` (ingress vs egress).
- **(c) Sample-dest parity.** `superset.sampleDest ⊇ subset.sampleDest` (MIRROR vs CPU).
- **(d) Sampled-port-scale containment.** `superset.sampledPortScale ⊇ subset.sampledPortScale` (one-port vs all-ports).
- **(e) Truncation-kind parity.** `superset.truncationKind ⊇ subset.truncationKind` (config-flag / ingress-feature / egress-feature / dataplane-byte-check).
- **(f) Verification-family parity.** Do not fold across `verificationFamily` (the universal Gate 4 HW-state/dataplane split — for mirror this also blocks the cross-file folds outright).

## Worked notes (from the report-mode run)

- File A (`AgentHwMirrorTests`) is `verificationFamily=hwState` (`trafficMode=none`); File B (`AgentMirroringTests`) is `dataplane`. Predicate (f) blocks every cross-file fold (40 macros, 0 cross-file candidates).
- The only human-review-worthy pairs were the `Update*`-vs-create pairs in File B: the `Update` variant changes mirror-to-port/dscp, so the original binding (a `portBindings` entry) is never re-verified — likely a real coverage delta. A validated pack would capture this via `portBindings`.
- Last-owners kept under Gate 1: `EGRESS_MIRROR_PACKET_TRUNCATION` (`TruncatePortErspanMirror` egress), `ERSPANv4_SAMPLING`/`ERSPANv6_SAMPLING` (`ErspanIngressSampling`/`SamplePacketFormat`), `INGRESS_ACL_MIRRORING`, `EGRESS_MIRRORING`.
