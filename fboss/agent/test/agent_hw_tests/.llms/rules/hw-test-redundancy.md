---
description: Flag a new/modified agent_hw_test that is redundant with an existing test (coverage already covered) — advisory, never blocks
oncalls:
  - fboss_agent
apply_to_regex: fbcode/fboss/agent/test/agent_hw_tests/.*\.cpp$
apply_to_clients: ['code_review']
---

# FBOSS Redundant HW Test review

On-diff expert review for `agent_hw_tests`. Every HW test runs on real ASICs in the lab on every run, so a test whose hardware coverage is already covered by another test is wasted lab time. This rule is applied inline by the code-review client (no subagents). It is **advisory**: it never blocks landing, never auto-applies, and is biased toward keeping coverage — when subsumption is not clear, stay silent.

---

## [HWTEST-001] New/modified test redundant with an existing test
**Severity:** warning
**Description:** A newly added or modified `TEST_F`/`TEST_P`/`TYPED_TEST` in this diff whose hardware coverage is fully contained in an existing test (or jointly in a set of existing tests) adds no coverage and should be folded into the existing test instead of added.

**Detection (candidate-vs-corpus):** The candidate is each test macro ADDED or MODIFIED by the diff. The potential supersets are the EXISTING tests in the same file/area (read the file). The candidate is redundant only if an existing test — or a union of existing tests — covers everything the candidate does. Judge by what the candidate *programs* and *verifies*, not by text similarity. Compare one fingerprint per macro (for `TEST_P`/`TYPED_TEST`, union the coverage across instantiations, including production-feature tags). The candidate is redundant only if a superset passes ALL of:

- **Production features:** the superset's `getProductionFeaturesVerified()` ⊇ the candidate's. If the candidate is the only test attesting some `ProductionFeature`, it is NOT redundant.
- **ASIC/SDK:** the superset runs on every `(ASIC, SDK)` the candidate runs on (respect `#if` SDK guards, vendor branches, `isSupportedOnAllAsics`).
- **Warmboot shape:** same `verifyAcrossWarmBoots` stages. A 2-arg test cannot cover a 4-arg test; two 4-arg tests must match per-stage (which mutation runs cold vs post-warmboot, and `direction`). An empty cold `verify` means the coverage is warmboot-only.
- **Verification depth per object:** the superset verifies at least as deeply on every object the candidate touches, on every axis the candidate uses. HW-state introspection (reading programmed state) and dataplane/traffic checks are orthogonal — one never covers the other. Direction matters: a candidate that verifies a SUBSET of what an existing test asserts (e.g. programs two stats but checks only one, while an existing test programs both and checks both) is strictly WEAKER, not distinct — it is subsumed. "New verification depth" that warrants keeping means the candidate verifies an object or axis the superset does NOT (a deeper property, or an added dataplane check) — never that it verifies FEWER assertions.
- **Fixture mode:** same runtime mode(s) — pinned via `setCmdLineFlagOverrides()` vs inherited platform default (e.g. `FLAGS_enable_acl_table_group` single/multi, `FLAGS_multi_switch` mono/mnpu). A test locked to a mode the other never runs is not interchangeable.
- **Test semantics:** same polarity (positive/negative), mutation/idempotency sequence, scale/count intent, atomic-vs-incremental config-apply, and the same hardware root-cause behind any shared counter (a counter bumped from a different cause — e.g. unresolved-binding drop vs terminal/no-match drop — is distinct coverage).

**Pick the correct superset by what the candidate PROGRAMS, not only what it asserts.** A candidate that programs two objects but verifies one is covered by a test that programs both AND verifies both (the superset's assertions are a superset of the candidate's). Do NOT treat the candidate's under-verification as distinct coverage — it is weaker, hence subsumed. Use a test that programs both, not a smaller test that programs only one.

**Flag (advisory):** "new test `X` appears subsumed by existing `Y` — consider folding into `Y` instead of adding a separate test." Name `Y` (or the joint set `Y` + `Z`).

**Stay silent (do NOT flag) when:**
- The candidate adds any coverage no existing test has — a new qualifier/value-class, action param, counter type, warmboot stage, `(ASIC, SDK)` combo, fixture mode, polarity, scale point, or drop cause.
- The candidate is the last owner of any production feature.
- Subsumption cannot be established with confidence (bias to keeping coverage).

**Never** recommend removing an EXISTING test (out of scope for on-diff review) — only advise folding the incoming one. Never block landing.

---

## Notes

- Confidence is highest for ACL (a validated area). For areas whose coverage pack is still drafted/unvalidated (ecmp, mirror, trunk, srv6) or absent (qos, copp), treat findings as lower-confidence nudges.
- Full rubric and per-area coverage packs (for deeper / human analysis): see the `hw-test-dedup` skill (sibling under `.llms/skills/`) — its universal-gates, fingerprint-schema, coverage-packs, and warmboot-semantics references.
