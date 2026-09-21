# FSDB & thrift_cow Patterns

Patterns for code under `fboss/fsdb/`, `fboss/thrift_cow/`, and FSDB client usage.

### 1. State/Stats Duality
**Check**: FSDB operations must handle both the state tree and the stats tree. When adding a new subscription, publisher, or path handler, ensure both `OperState` and `OperStats` are covered.
**Why**: Handling only one tree causes missing telemetry (stats) or missing config reconciliation (state).
**Confidence**: HIGH (always a bug if only one tree is handled)

### 2. extern template for ThriftStructNode
**Check**: New `ThriftStructNode` instantiations must have corresponding `extern template` declarations in the header and explicit instantiations in a `.cpp` file.
**Why**: Without `extern template`, every translation unit that includes the header instantiates the template, causing O(n) build time regression.
**Confidence**: HIGH (always required for new ThriftStructNode types)

### 3. Subscription Type: Prefer Patch
**Check**: New FSDB subscriptions should use `PatchSubscription` (preferred), not `DeltaSubscription` (deprecated).
**Why**: Delta subscriptions are being deprecated. Patch subscriptions are more efficient and support partial updates.
**Confidence**: MEDIUM (existing delta subscriptions don't need immediate migration, but new code should use patch)

### 4. COW Node Modification
**Check**: To modify a COW (copy-on-write) node, always use `modify()` to get a mutable copy. Never cast away const or mutate a shared node directly.
**Why**: Mutating a shared node corrupts other references to the same node, causing data races and state corruption.
**Confidence**: HIGH (always a bug)

### 5. Publisher Queue Bounds
**Check**: FSDB publishers must respect queue size bounds. When publishing updates, check that the queue is not full before enqueuing.
**Why**: Unbounded publishing can cause memory exhaustion in the FSDB server process.
**Confidence**: MEDIUM (depends on publish frequency and data size)
