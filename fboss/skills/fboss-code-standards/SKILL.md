---
name: fboss-code-standards
description: FBOSS coding standards and patterns. Auto-loaded when writing code in fboss/ to catch architecture violations, SAI/SDK misuse, thrift_cow pitfalls, platform config errors, and testing gaps. For explicit multi-reviewer review, use /fboss-review instead.
---

# FBOSS Code Standards

## Overview

Passive coding guidance for FBOSS. Applied automatically while writing or modifying code under `fboss/`.

## Scope

Currently `fboss/` only. TODO: extend to `configerator/source/neteng/fboss`, `neteng/netcastle`, `neteng/fboss`.

## Quick Checklist

| Area | Pattern | Check |
|------|---------|-------|
| Agent | Mono/multi-switch duality | State changes must work in both modes |
| Agent | Warmboot serialization | New SwitchState fields must serialize/deserialize |
| SAI | SaiApiTable registration | New SAI attributes must be registered |
| SAI | SaiStore consistency | SAI objects must be tracked, no orphans |
| FSDB | State/Stats duality | Always handle both trees |
| FSDB | extern template | New ThriftStructNode instantiations need extern template |
| thrift_cow | COW modification | Use `modify()`, never mutate shared nodes |
| Platform | JSON + Thrift sync | Config changes update both schemas |
| Testing | Naming convention | Follow `AgentHw<Feature>Test` pattern |
| Testing | NSDB impact | Core FSDB changes must run NSDB tests |
| General | Follow local patterns | Be consistent with existing code in the directory |
| Testing | No GTEST_SKIP | Use ProductionFeatures filtering, never `GTEST_SKIP()` |
| Testing | DSF counters | Check reassembly errors on fabric ports, not discards |
| Agent | Non-coalescing behavioral deltas | Mark delta-significant updates non-coalescing |
| Agent | Rolled-out flag cleanup | Remove feature flags that are fully rolled out |
| Agent | Early return on empty deltas | Check for empty deltas and return early |
| Agent | No hardcoded ASIC types | Use feature/property lookups, not ASIC name checks |
| SAI | Explicit cancellation | Distinguish reconnectable errors from shutdown signals |
| General | No private fn defaults | Don't default parameters all callers override |
| General | Explicit state flags | Pass all enable/disable flags, no direction assumptions |
| General | Verify before deleting | Provide evidence files are unused before deleting |
| General | CHECK over assert | Use CHECK for production invariants, not assert() |
| General | emplace over operator[] | Use emplace/insert for map insertions |
| General | ASIC feature gating | Gate HW-specific constraints with ASIC features |
| General | Concise method naming | Don't include class name in method name |
| General | Explicit operator precedence | Parenthesize compound boolean/comparison expressions |
| General | Inline trivial comparisons | Don't wrap one-line checks in helper methods |
| General | Fix lint before landing | Run `arc f` and `arc lint` before submitting |
| Testing | Targeted unit tests | New modules need dedicated unit tests, not just HW tests |
| Agent | Documentation-value methods | Keep per-ASIC limit methods as architecture docs |
| General | Rate-limit verbose logging | Use XLOG_EVERY_MS, never log at line rate |
| General | Perspective-aware naming | Use fromX/toX instead of ambiguous Rx/Tx |
| General | Update OSS build files | Update CMake alongside BUCK when adding/moving files |
| Testing | Deep comparison over size checks | Compare content, not just collection size |
| Testing | Verify negative cases | Explicitly check absence, not just no-throw |
| Testing | Test overflow and error paths | Cover resource exhaustion, not just happy path |
| Testing | Optimize test execution time | Use smallest packet sizes, avoid unnecessary waits |
| Agent | ODS counters for debugging | Add ODS counters for drop/error events, not just logs |
| Agent | Validate external config limits | Reject unreasonable externally configurable values |
| Agent | Populate warmboot cache first | Fill all caches from warmboot state before processing |
| General | Empty vector over optional\<vector\> | Use empty vector, not optional\<vector\> |
| General | Map-based config | Use map lookups, not if-else chains for platform values |
| General | Log IDs + names | Include both numeric ID and name in log messages |
| Agent | Delta processing order | Remove → Change → Add; document in comments |
| Agent | Validator extraction | Extract complex validation into dedicated classes |
| Agent | Gate validations with flags | New enforcement behind FLAGS for gradual rollout |
| Testing | Test transient states | Test intermediate invalid states, not just final |
| Testing | Consolidate test helpers | Move shared helpers to base classes |
| Testing | Validation UTs | Pure validation → unit tests, not just HW tests |
| Testing | No setup changes to existing tests | Create new tests instead; setup changes break warmboot roundtrip CI |
| Testing | Program routes via routeUpdater | Use `routeUpdater.program()` (`SwSwitchRouteUpdateWrapper`); never hand-build routes and add to FIB; keep `enable_nexthop_id_manager` on so every route gets a nexthop ID |
| Agent | Read nexthops via ID-aware helpers | State/FIB-side code → `FibHelpers` `getNextHops(state,…)` etc.; RIB-internal code → `*FromRib(manager,…)`; never `getNextHopSet()` (inline nexthops are being removed) |

## When to Load References

| Topic | Action |
|-------|--------|
| Team coding conventions (C++ style, control flow, naming) | Read `references/general-patterns.md` |
| Agent code (SwSwitch, HwSwitch, mono/multi-switch, warmboot) | Read `references/agent-patterns.md` |
| SAI/SDK layer (SaiApi, SaiStore, SaiManager, vendor SDK) | Read `references/sai-sdk-patterns.md` |
| FSDB or thrift_cow (subscriptions, COW nodes, PatchBuilder) | Read `references/thrift-cow-fsdb-patterns.md` |
| Platform services or config (sensor_service, fan_service, JSON configs) | Read `references/platform-config-patterns.md` |
| Tests (agent HW tests, multinode tests, naming, fixtures) | Read `references/testing-patterns.md` |
| Adding new patterns | Read `references/contributing.md` |
