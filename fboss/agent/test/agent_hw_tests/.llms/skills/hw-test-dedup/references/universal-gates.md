# The 6 universal gates

These six gates are the safety rubric for de-duplicating FBOSS `agent_hw_tests`. A candidate pair is a `(subset, superset)` proposal: the claim is that the superset's hardware coverage fully contains the subset's, so the subset can be removed without losing coverage. The subset is removable ONLY if the pair passes every gate below. Any single failure blocks the fold.

These gates are injected into both the subset-detector and the skeptic personas. The detector proposes folds; the skeptic must independently re-check each gate and reject on the first failure.

### Gate 1: Production-feature attestation

The superset must carry every `ProductionFeature` that the subset's fixture lists via `getProductionFeaturesVerified()`. A test that is the LAST owner of any feature (the only fixture attesting it) is non-removable, regardless of how well its body appears to be covered elsewhere.

A pair FAILS this gate if the subset declares any `ProductionFeature` not also declared by the superset, or if removing the subset would drop the last owner of any feature from the suite.

Enforcement: code (build the feature->owners map across all fixtures, then check containment and last-owner status mechanically).

### Gate 2: ASIC/SDK conditionality

Coverage is conditional on hardware and SDK. The superset's ASIC set must be a superset of the subset's: `superset.asicSet` must contain `subset.asicSet`. The check must respect `#if` SDK guards (e.g. `TAJO_SDK_GTE_*`), per-vendor branches, and `isSupportedOnAllAsics` gates. Subsumption must hold on every `(ASIC, SDK)` combination the subset actually runs on, not merely on the union.

A pair FAILS this gate if the subset runs on any `(ASIC, SDK)` combination where the superset does not run, or where the superset's coverage of the shared object is compiled out by an SDK guard or vendor branch that the subset does not have.

The source of truth for ASIC/SDK support is `fboss/oss/production_features/asic_production_features.materialized_JSON`.

Enforcement: code + agent (code resolves the ASIC/SDK support sets from the materialized JSON; the agent reasons about `#if` guards and vendor branches the JSON cannot express).

### Gate 3: Warmboot shape

A warmboot test has a per-stage shape: `{argForm, coldSetup, coldVerify, postWbSetup, postWbVerify, direction}` (one list per `verifyAcrossWarmBoots` callback — see `warmboot-semantics.md`). Compare the stages structurally, not as a single label. A 2-arg `verifyAcrossWarmBoots(setup, verify)` cannot subsume a 4-arg `verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot)`, because the 4-arg form exercises a post-warmboot mutate-and-verify phase the 2-arg form never runs. But "both 4-arg" is NOT sufficient: two 4-arg tests are foldable only if their per-stage token lists match — they can differ in mutation ordering, in which stage holds the assertion, or in `direction`. Coverage placed only in `coldSetup` (cold-boot only) or only in `postWbSetup`/`postWbVerify` (warmboot only) is a trap: it executes on just one boot type. An EMPTY `coldVerify` is the loudest signal of this — the test's coverage lands entirely on the warmboot pass.

A pair FAILS this gate if the warmboot shapes differ in any stage's tokens or in `direction`, or if the superset's arg-form or per-stage coverage placement means it does not exercise the subset's coverage on the same boot type(s).

Enforcement: agent (informed by `warmboot-semantics.md`).

### Gate 4: Verification depth, per shared object

For each object both tests touch, verification depth must be compared per object. The two axes are area-agnostic: **HW-state introspection** (read back programmed state via the HW-agent test client) and **dataplane/forwarding checks** (send traffic, read counters/queues). They are ORTHOGONAL: passing one says nothing about the other. (ACL examples of the two axes: HW-state — `isAclTableEnabled`, `isAclEntrySame`, `getAclTableNumAclEntries`; dataplane — `getAclInOutPackets`, queue-out counts.) The superset must verify at least as deeply as the subset on each shared object, along every axis the subset uses.

A pair FAILS this gate if, for any shared object, the subset verifies along an axis (HW-state or dataplane) where the superset does not, or verifies more deeply along an axis than the superset does.

Enforcement: agent.

### Gate 5: Fixture-mode parity

The same `.cpp` can run in different runtime modes selected by a command-line flag, and each mode exercises a different code/HW path. This is area-agnostic — examples: mono vs multi-switch via `FLAGS_multi_switch` (`agent/single` vs `agent/mnpu`); SAI vs native via `isSai()`; and, for ACL, single vs multi table via `FLAGS_enable_acl_table_group` (the specific flag and mode names belong in that area's coverage pack, not here). A fixture either PINS a mode via `setCmdLineFlagOverrides()`, or INHERITS the platform default (`defaultCommandLineArgs` in the materialized agent config, plus netcastle `agent_config` overrides). The superset's mode set must contain the subset's: `superset.modeSet` must contain `subset.modeSet`. A pairing where one fixture is locked to a mode the other never runs is rejected. If the mode of either fixture cannot be resolved confidently, refuse the cross-mode fold rather than guess.

A pair FAILS this gate if the subset runs in any mode the superset does not, if the two are locked to disjoint modes, or if either mode cannot be resolved with confidence.

Enforcement: code + agent (code resolves pinned/inherited modes from the flag overrides and agent config; the agent adjudicates ambiguous cases and refuses on low confidence).

### Gate 6: Test semantics

Distinct test intents are distinct coverage even when they touch the same objects with the same depth. These are area-agnostic: negative/polarity intent (positive vs negative — e.g. for counters, bump vs no-bump; generally match vs no-match), mutation/idempotency sequences (add / modify / del / re-add), scale/count intent, atomic-vs-incremental config-apply (one `applyNewConfig` vs two), and the hardware root-cause behind a shared counter/stat. On that last point: two tests that drive the SAME observable counter or stat from DIFFERENT hardware causes are distinct coverage even at identical polarity and depth — e.g. a drop counter incremented by an unresolved-binding drop vs a terminal/no-next-hop drop, or a resource stat asserted in a resolved vs an unresolved state. A shared counter name is not evidence of shared coverage; the stimulus that drives the counter must match. Never fold across any of these unless the intent matches.

A pair FAILS this gate if the subset and superset differ in polarity, mutation/idempotency sequence, scale/count intent, or config-apply granularity, or if they assert the same counter/stat driven by a different hardware cause.

Enforcement: agent.

---

Beyond these, each area contributes extra predicates via its coverage pack (see `coverage-packs/`).
