# ACL coverage pack

A coverage pack adds per-feature-area fingerprint fields and gate predicates that the de-dup tool applies *on top of* the 6 universal gates. This pack is validated and `--apply`-eligible for the ACL feature area.

## Extra fingerprint fields

Beyond the universal fingerprint, fingerprint an ACL test on:

- **Qualifier tuples** — the *combination* of qualifiers exercised per ACL entry, not the flat set. Record each entry's qualifier tuple, e.g. `{dscp}`, `{ttl}`, `{srcPort, l4DstPort}`. A test that pairs two qualifiers in one entry is distinct from two tests that each exercise one qualifier alone.
- **Qualifier value-classes** — for each qualifier, the class of value(s) exercised: boundary / min / max / masked, and v4 vs v6 width. `{l4DstPort=0}` (min), `{l4DstPort=65535}` (max), and a masked match are distinct value-classes of the same qualifier type.
- **Action params** — `setDscp` value, queue id, counter sharing mode, and PERMIT vs DENY. Two tests on the same qualifier tuple but different actions are not duplicates.
- **Entry priority / ordering / overlap** — entry priority values, relative ordering, and whether entries overlap (match-region intersection). Overlap resolution is HW behavior that non-overlapping tests do not exercise.
- **Counter types** — PACKETS / BYTES / both. Counter-type coverage is independent of the qualifier and action coverage.
- **Config-apply path** — atomic (tables + entries in one `applyNewConfig`) vs incremental (two-delta: tables first, then entries). The HW programming path differs between the two.

## Extra gate predicates

A candidate ACL test is subsumed only if, in addition to the 6 universal gates passing:

- **(a) Qualifier tuples + value-classes match.** Matching the *set of qualifier types* is insufficient. The superset must exercise the same qualifier tuples AND the same value-classes (boundary/min/max/masked, v4/v6 width) as the candidate.
- **(b) Priority/overlap is not subsumed by non-overlapping.** A test with overlapping or priority-dependent entries is never subsumed by a test whose entries do not overlap, even if qualifiers and actions match.
- **(c) Atomic vs incremental create are distinct.** An atomic single-delta create and an incremental two-delta create are distinct unless some *other* retained test exercises both paths for the same configuration.
- **(d) ACL-table-mode parity (SINGLE vs MULTI).** Tests must match on ACL table mode; cross-reference universal Gate 5 and the "Fixture mode (ACL)" section below. A candidate run in one table mode is not subsumed by a superset run only in the other mode.
- **(e) Traffic-injection path is not distinct for counter-bump tests.** Per the "Traffic-injection-path rule" below, a front-panel bump test subsumes the equivalent CPU bump test; collapse the CPU variant.

## Fixture mode (ACL)

For ACL, the universal Gate 5 runtime mode (`fixtureMode`) is the ACL-table layout selected by `FLAGS_enable_acl_table_group`; the area-specific mode tokens are `single` and `multi`.

- The table-group fixtures (`AgentAclTableGroupTest`, `AgentAclTableGroupTrafficTest`) PIN multi via `setCmdLineFlagOverrides()` → `pinned:multi`.
- The stat / counter / match-action fixtures INHERIT the platform default → `inherited:single,multi` (whatever the platform + netcastle `agent_config` set).
- UDF, flowlet/DLB, and BTH-opcode tests are single-table only → `single` (`pinned:single`).

A `pinned:multi` test and a `single`-only test are locked to disjoint modes and are not interchangeable (Gate 5).

## Worked notes (ACL run)

From the ACL de-dup run on the HW-test corpus:

- **VerifyUdf, VerifyFlowlet, VerifyUdfFlowlet** were in-isolation variants subsumed by their `*WithOtherAcls` supersets. The superset verifies the same ACL is bumped *and* co-resident with a `SRC_PORT` ACL, so the in-isolation variant adds no coverage the superset lacks.
- **MultipleTablesWithEntries** was a union-subset: `AddTablesThenEntries` covers the HW-state assertions and `VerifyQueuePerHostAclTableAndTtlAclTable` covers the atomic-create path, so the two supersets together subsume it.
- **VerifyL4DstPortRangeAcl** was kept — it is the sole owner of the `L4_DST_PORT_RANGE` qualifier, so no superset subsumes it.

## Traffic-injection-path rule (CPU vs front-panel)

For counter-bump tests, the packet **injection path** (CPU-originated vs front-panel) is **not** a distinct HW-coverage dimension: a front-panel bump test subsumes the equivalent CPU bump test, because both exercise the same ACL match + counter HW path and differ only in where the packet originates. Recommend collapsing a CPU bump test into its front-panel counterpart.

This does **not** apply to negative/no-bump tests, priority/ordering tests, or scale tests.
