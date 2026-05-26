# SAI/SDK Layer Patterns

Patterns for code under `fboss/agent/hw/sai/`, `fboss/lib/phy/`, and vendor SDK integrations.

### 1. SaiApiTable Registration
**Check**: New SAI attributes must be registered in the corresponding `SaiApiTable` entry. Check that the attribute is added to the attribute list in the relevant `SaiApi` (e.g., `SaiPortApi`, `SaiRouteApi`).
**Why**: Unregistered attributes are silently ignored, causing features to appear configured but not actually programmed in hardware.
**Confidence**: HIGH (always a bug)

### 2. SaiStore Consistency
**Check**: SAI objects must be tracked in `SaiStore`. Creating a SAI object without adding it to the store, or removing it without cleaning the store entry, creates orphans.
**Why**: Orphaned SAI objects leak hardware resources and cause inconsistency during warmboot reconstruction.
**Confidence**: HIGH (always a bug)

### 3. SAI Attribute Mapping
**Check**: When adding new Thrift fields that map to hardware configuration, ensure the corresponding SAI attribute mapping exists in the `SaiManager` (e.g., `SaiPortManager`, `SaiRouteManager`).
**Why**: Missing mappings mean the Thrift config is accepted but never programmed, causing silent configuration failures.
**Confidence**: HIGH (always a bug if the field is meant to be programmed)

### 4. SDK Version Guards
**Check**: Vendor-specific SAI behavior or attributes must be guarded by SDK version checks (e.g., `#if defined(MEMORY_SAI_VERSION_X_Y_Z)`).
**Why**: Using attributes not supported by older SDK versions causes compilation failures or runtime crashes on platforms with older SDKs.
**Confidence**: HIGH (always required for vendor-specific features)

### 5. SAI Test Coverage
**Check**: New SAI features need both `SaiManagerTest` (unit test for manager logic) and `AgentHwTest` (integration test on real/simulated hardware).
**Why**: Manager tests alone don't verify hardware interaction; HW tests alone don't verify state management logic.
**Confidence**: MEDIUM (test coverage is important but some features may only need one type)

### 6. Avoid Static/Thread-Local Buffers in SaiAttribute _fill Calls
**Check**: `_fill` functions that convert between SAI list types and C++ value types must not use `static` or `thread_local` intermediate buffers. Instead, choose a C++ value type whose memory layout matches the SAI type so data can be pointed to directly.
**Why**: Static/thread-local buffers are shared across calls. When multiple objects are serialized (e.g., during warmboot adapter host key serialization), the buffer is overwritten on each call, so only the last value survives. This caused a real bug (D98397282) where SRV6 segment lists were corrupted — only the final segment list value appeared in warmboot state. The fix was to change `SaiSrv6SidListTraits::SegmentList` from `std::vector<folly::IPAddressV6>` to `std::vector<std::array<uint8_t, 16>>`, which has the same memory layout as `sai_ip6_t` and can be pointed to directly without any conversion buffer. Type conversions (e.g., from `folly::IPAddressV6`) should happen at the caller boundary using helpers like `toSaiIp6List()` in `AddressUtil`.
**Confidence**: HIGH (caused a warmboot data corruption bug)

### 7. Keep Locking Contained in SaiSwitch
**Check**: Locking logic (mutexes, lock guards, lock policies) must not leak into individual `SaiManager` classes (e.g., `SaiNextHopManager`, `SaiFdbManager`). Concurrency control should remain constrained to `SaiSwitch` — the rest of the SAI switch layer code should not need to worry about concurrency.
**Why**: The SAI switch layer is designed so that `SaiSwitch` owns the `saiSwitchMutex_` and manages all locking via `FineGrainedLockPolicy`. If individual managers start accepting lock guards or lock policies in their APIs, locking responsibilities become distributed and hard to reason about, increasing the risk of deadlocks and lock-ordering violations. This was flagged in D92985792 where a change attempted to pass lock policies into manager `listManagedObjects` calls — the correct approach is to acquire and release locks within `SaiSwitch` methods and call manager APIs without exposing locking to them.
**Confidence**: HIGH (architectural invariant)

### 8. Never Silently Ignore SwitchState Programming Requests
**Check**: SAI Manager code must not use `if` conditions that silently skip programming a field from `SwitchState`. If the control plane asks the SAI layer to program something (ACL qualifier, next hop attribute, port config, etc.), the SAI layer must either program it or throw an error — never silently ignore it.
**Why**: Silent skips mask bugs. When a SwitchState delta requests a configuration change and the SAI layer silently drops it because a field is unset or an ASIC capability is missing, the system reports success while the hardware is misconfigured. This is especially dangerous with vendor-contributed code that may implement partial support — the feature appears to work in review but silently fails on certain inputs. The correct behavior is to throw `FbossError` or `SaiApiError` when a requested configuration cannot be programmed, so the failure is visible. Do not accept PRs from vendors that add `if (!field.has_value()) { return; }` guards that skip programming — these hide incomplete implementations. (Flagged in D104291402)
**Confidence**: HIGH (silent config drops are always a bug)

### 9. Explicit Cancellation for Thrift/Stream Clients
**Check**: Reconnecting Thrift and stream clients must use explicit cancellation signals, not treat all exceptions as cancellation. Disconnections during normal operation should trigger reconnection, not shutdown.
**Why**: Treating any exception as cancellation causes clients to permanently stop when they should reconnect. Only explicit cancellation (e.g., shutdown signal) should terminate the client.
**Confidence**: HIGH (always distinguish reconnectable errors from shutdown signals)
