# Agent Architecture Patterns

Patterns for code under `fboss/agent/`, including SwSwitch, HwSwitch, and state management.

### 1. Mono/Multi-Switch Duality
**Check**: Any state change or control path must work in both mono mode (`FLAGS_multi_switch=false`, `agent/single/`) and multi-switch mode (`FLAGS_multi_switch=true`, `agent/mnpu/`)
**Why**: FBOSS supports both single-process and multi-process architectures. Code that only works in one mode causes production failures on the other deployment type.
**Confidence**: HIGH (always a bug if only one mode is handled)

### 2. Warmboot State Preservation
**Check**: New fields added to SwitchState or its child nodes must be serialized and deserialized during warmboot. Verify the field appears in the warmboot state Thrift structure and is handled in `fromThrift()`/`toThrift()`.
**Why**: Missing serialization causes state loss during warmboot, leading to traffic disruption or config drift after agent restart.
**Confidence**: HIGH (always a bug if new state fields are not serialized)

### 3. OperDelta Sync in Multi-Switch
**Check**: In multi-switch mode, HwSwitch state updates must go through OperDelta sync (`OperDeltaSyncer`), never by direct HwSwitch mutation from SwSwitch.
**Why**: Direct mutation bypasses the Thrift IPC boundary, causing state divergence between SwSwitch and HwSwitch processes.
**Confidence**: HIGH (always a bug in multi-switch mode)

### 4. State Delta Ordering
**Check**: When applying state deltas, respect dependency order: VLANs before interfaces, interfaces before routes, ACLs before applying to interfaces.
**Why**: Out-of-order delta application causes transient failures or crashes when referenced objects don't exist yet.
**Confidence**: HIGH (ordering violations cause crashes)

### 5. Lock Ordering in SwSwitch
**Check**: When acquiring multiple locks, follow the established lock hierarchy. Never hold `updateLock_` while acquiring `stateLock_`; `stateLock_` must be acquired first if both are needed.
**Why**: Lock ordering violations cause deadlocks that hang the agent.
**Confidence**: HIGH (always a bug)

### 6. Non-Coalescing Updates for Behavioral Deltas
**Check**: State updates where the delta carries behavioral significance (e.g., deltaApplicationBehavior is set) must use non-coalescing update paths. Do not allow the state update framework to coalesce such deltas.
**Why**: Coalesced updates collapse intermediate states. A sequence of enable→disable→enable is different from just enable, but coalescing reduces it to the final state, losing behavioral intent.
**Confidence**: HIGH (always a bug if coalesced when delta behavior matters)

### 7. Clean Up Fully Rolled-Out Feature Flags
**Check**: When a feature is fully rolled out across all platforms, remove the feature flag and the dead code path. Do not leave conditional checks for flags that always evaluate to true.
**Why**: Dead flag branches add complexity, confuse readers, and accumulate debt. If every platform has the feature, the flag serves no purpose.
**Confidence**: MEDIUM (requires confirming rollout is complete on all platforms)

### 8. Early Return on Empty Deltas
**Check**: Delta processing functions must check for empty deltas at the start and return early. Do not process all state updates when only a specific delta changed.
**Why**: Processing empty deltas wastes CPU and can trigger unnecessary side effects (e.g., recomputing local switch IDs on every state update instead of only when DSF node map changes).
**Confidence**: HIGH (always add an early return for empty deltas)

### 9. Don't Hardcode ASIC Types
**Check**: Do not check for specific ASIC types (e.g., `if asicType == J3`) when the information can be derived from config, DSF node map, or ASIC feature flags. Look up ASIC properties dynamically.
**Why**: Hardcoded ASIC checks break when new ASICs are added and create maintenance burden. Use feature-based gating instead.
**Confidence**: HIGH (always use feature/property lookups over hardcoded ASIC names)

### 10. Retain Documentation-Value Per-ASIC Methods
**Check**: Keep per-ASIC limit methods or constants that document platform-specific capabilities (e.g., max neighbor entries per ASIC), even when a unified gflag-based limit is used at runtime. Do not delete them during cleanup unless the platform is fully deprecated.
**Why**: These methods serve as architecture documentation. Without them, future engineers must reverse-engineer per-platform limits from scattered config files or vendor docs.
**Confidence**: MEDIUM (requires judgment on whether the platform is still relevant)

### 11. Use ODS Counters for Production Debugging
**Check**: Add ODS counters (not just logs) for events that need production debugging — packet drops, errors, resource exhaustion. Add follow-up configerator diffs to permit export of new counters to ODS.
**Why**: Logs are ephemeral and hard to aggregate. ODS counters provide exact counts, time-series trending, and alerting. Without counters, production packet drops require log scraping to debug.
**Confidence**: HIGH (always add ODS counters for drop/error events)

### 12. Validate External Configuration Limits
**Check**: Validate externally configurable limits (thrift config, gflags, configerator) to prevent values that could destabilize the system. Reject unreasonable values (e.g., 1M pkts/ms rate limit) that would undo built-in protections like thrift overload guards.
**Why**: External users can configure arbitrary values. Without bounds checking, a misconfiguration can cause CPU exhaustion, memory exhaustion, or bypass overload protection.
**Confidence**: HIGH (always validate external config bounds)

### 13. Populate Warmboot Cache Before Processing
**Check**: During warmboot, populate all caches and ID maps from the warmboot state before processing any new state changes. Do not lazily discover IDs as routes or entries are added.
**Why**: Lazy discovery causes ID collisions — new allocations may reuse IDs that exist in the warmboot cache but haven't been discovered yet, corrupting forwarding state.
**Confidence**: HIGH (always populate warmboot caches before processing new state)

### 14. Delta Processing Order
**Check**: Process deltas in consistent order: removals → changes → additions (per interface, per object type). Document the ordering in comments.
**Why**: Wrong order creates transient invalid states (e.g., two neighbors with same MAC during update). Documented order prevents future refactors from breaking it.
**Source**: D96253394 reviewer feedback
**Confidence**: HIGH

### 15. Extract Validation into Dedicated Classes
**Check**: Complex validation logic should be extracted into dedicated validator classes with clear `isValidDelta()` and `stateChanged()` APIs, not embedded in state update handlers.
**Why**: Single responsibility, easier to unit test in isolation, easier to maintain ordering semantics.
**Source**: D96253394 reviewer feedback
**Confidence**: MEDIUM

### 16. Gate New Validations Behind Feature Flags
**Check**: New validation/enforcement logic must be gated behind a FLAGS (e.g., `FLAGS_enforce_single_nbr_mac_per_intf`) for gradual rollout.
**Why**: Allows disabling if issues arise in production, enables A/B comparison, prevents big-bang rollout risk.
**Source**: D96253394 reviewer feedback
**Confidence**: HIGH

### 17. Rebuild Validator State on Rejection
**Check**: When `updateRejected()` is called after partial validation, rebuild validator state from scratch via `stateChanged(empty → oldState)` rather than attempting incremental undo.
**Why**: Incremental undo is error-prone. Clean rebuild ensures validator state stays in sync even when validation fails partway through.
**Source**: D96253394 reviewer feedback
**Confidence**: MEDIUM

### 18. Read Route NextHops Through the ID-Aware Helpers (FIB vs RIB)
**Check**: Never read a route's nexthops via `getNextHopSet()` (or otherwise read inline nexthop sets directly off the route/fwd-entry object) in new or modified code — inline nexthops are being removed by the ECMP NextHop-ID migration. Use the ID-aware helper family matched to your layer (both in `namespace facebook::fboss`):
- **Code with a published `SwitchState`** — the default for everything outside the RIB internals (SaiSwitch, SaiRouteManager, resolvers, thrift handlers, ResourceAccountant, ERM, emulation switch, tests) → use the `FibHelpers.h` state-based helpers: `getNextHops(state, id)` / `getNextHops(state, entry)`, `getNormalizedNextHops(state, entry)`, `getNonOverrideNormalizedNextHops(state, entry)`, `getClientNextHops(state, entry)`.
- **RIB-internal code that runs before the state is published** (has only a `NextHopIDManager*`, e.g. inside `RibRouteUpdater` / `RoutingInformationBase`) → use the `RoutingInformationBase.h` `*FromRib` helpers: `getNextHopsFromRib(manager, id)`, `getResolvedNextHopsFromRib(manager, entry)`, `getNormalizedNextHopsFromRib(manager, entry)`, `getNonOverrideNormalizedNextHopsFromRib(manager, entry)`, `getClientNextHopsFromRib(manager, entry)`. Each is the 1:1 companion of the FibHelpers function of the same name.
**Why**: Once inline nexthops are dropped, `getNextHopSet()` returns empty/stale data. The helpers resolve nexthops through the refcounted ID map and stay correct under `FLAGS_enable_nexthop_id_manager` / `FLAGS_resolve_nexthops_from_id`. Picking the wrong family is a layering error: FIB helpers need a published state the RIB doesn't have yet, and RIB helpers need the `NextHopIDManager` directly.
**Confidence**: HIGH (read-path code must go through the ID-aware helper for its layer)
