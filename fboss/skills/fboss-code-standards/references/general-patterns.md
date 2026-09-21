# General Coding Patterns

Team-wide coding conventions for FBOSS. These apply across all FBOSS subsystems.

### 1. Switch-Case Over Long If-Else Chains
**Check**: When branching on an enum or discrete value with 3+ cases, use switch-case instead of if-else
**Why**: Switch-case gives compiler warnings for unhandled enum values, preventing silent bugs when new values are added
**Confidence**: MEDIUM (context-dependent, some cases warrant if-else)

### 2. XLOG Over cout/printf
**Check**: Use `XLOG()` macros (XLOG_INFO, XLOG_ERR, XLOG_DBG) instead of `std::cout`, `printf`, or `LOG()`
**Why**: XLOG integrates with folly logging, supports structured logging, and can be filtered by category
**Confidence**: HIGH (always use XLOG in FBOSS code)

### 3. Consistent Error Reporting
**Check**: Use `FbossError` or `FbossHwError` for FBOSS-specific exceptions, not generic `std::runtime_error`
**Why**: FBOSS exception types carry additional context (switch index, error code) and integrate with error handling infrastructure
**Confidence**: HIGH (always use FBOSS exception types)

### 4. Const Correctness
**Check**: Mark methods, parameters, and variables `const` when they do not modify state
**Why**: Prevents accidental mutation, enables compiler optimizations, documents intent
**Confidence**: MEDIUM (follow local conventions in the file)

### 5. Follow Local Naming Conventions
**Check**: Match the naming style of surrounding code (camelCase for methods, camelCase for variables, UPPER_SNAKE for constants)
**Why**: Consistency within a file/directory is more important than a global rule
**Confidence**: HIGH (always match local style)

### 6. No Default Parameters on Private Functions
**Check**: Private/internal functions where all callers always pass the argument explicitly should not have a default value. If every caller overrides the default, the default is misleading.
**Why**: Defaults on private functions hide intent. Readers assume the default is common, but if all callers override it, the "default" is never used.
**Confidence**: MEDIUM (context-dependent; defaults on public APIs are fine)

### 7. Pass All State Explicitly
**Check**: When configuring state with multiple directions or modes (e.g., RX/TX, enable/disable), pass all flags explicitly. Do not hardcode assumptions like "only RX is enabled."
**Why**: Hardcoded assumptions break when the other direction is later enabled. Use standard SAI attributes or explicit parameters for all directions.
**Confidence**: MEDIUM (depends on whether the assumption is documented)

### 8. Verify Before Deleting
**Check**: When deleting files (especially via codemods), verify they are actually unused. Provide evidence: `buck targets`, dependency checks, or grep results.
**Why**: Automated codemods can incorrectly identify files as unused. Blindly trusting a codemod and deleting active files causes build breaks.
**Confidence**: HIGH (always provide evidence before approving file deletions)

### 9. CHECK Over assert for Runtime Invariants
**Check**: Use `CHECK` / `CHECK_EQ` (folly) for invariants that must hold in production. Do not use `assert()` for runtime safety checks — assert only triggers in dbg/dev builds and is compiled out in opt builds.
**Why**: Assertions in production builds are no-ops. Critical invariant violations go undetected, leading to silent corruption or crashes later.
**Confidence**: HIGH (always use CHECK for production safety checks)

### 10. Prefer emplace/insert Over operator[] for Maps
**Check**: When inserting into a map, use `emplace()` or `insert()` instead of `operator[]`. The latter default-constructs the value then assigns, wasting a construction.
**Why**: `operator[]` creates a default object and then copy/move-assigns. `emplace` constructs in place. For complex value types, this avoids unnecessary work. `insert` also returns whether the insertion happened, useful for logging.
**Confidence**: MEDIUM (performance impact depends on value type complexity)

### 11. Gate Hardware-Specific Constraints with ASIC Features
**Check**: Hardware-specific constraints (e.g., single MAC per port RIF) must be gated by ASIC/ProductionFeature checks, not assumed globally. Different ASICs (J2 vs J3AI vs Spectrum4) have different capabilities.
**Why**: Applying constraints globally either breaks platforms that support the feature or leaves unsupported platforms unprotected.
**Confidence**: HIGH (always gate with ASIC feature checks)

### 12. Concise Method Naming
**Check**: Method names should not redundantly include the class or module name. For example, a method in `ResourceAccountant` should be `getMaxNeighborTableSize()` not `getMaxNeighborTableSizeByResourceAccountant()`.
**Why**: Redundant context makes names unwieldy and harder to read. The class name already provides context at the call site.
**Confidence**: HIGH (always trim redundant prefixes/suffixes)

### 13. Explicit Operator Precedence
**Check**: Parenthesize compound boolean and comparison expressions for clarity, even when operator precedence produces the correct result. E.g., `isValidUpdate &= (count <= getMaxSize());` not `isValidUpdate &= count <= getMaxSize();`.
**Why**: Relying on implicit precedence in compound expressions forces the reader to recall precedence rules. Parentheses make intent explicit and prevent subtle bugs during future edits.
**Confidence**: HIGH (always parenthesize compound expressions)

### 14. Inline Trivial Comparisons
**Check**: Do not wrap one-line comparisons in helper methods. Inline the expression at the call site when it is equally readable. E.g., `count <= getMaxSize()` is more readable than `checkResource(count)` when the helper does nothing but that comparison.
**Why**: Unnecessary helper methods add indirection without abstraction benefit. The reader must jump to the helper to understand a trivial check.
**Confidence**: MEDIUM (judgment call — helpers are fine when they encapsulate non-trivial logic)

### 15. Fix Lint Before Landing
**Check**: All lint errors and warnings must be resolved before landing a diff. Run `arc f` (format) and `arc lint` before submitting. Do not land with known lint issues.
**Why**: Accumulated lint creates broken-window syndrome. Each unlinted diff makes it harder for the next author to distinguish their lint from pre-existing issues.
**Confidence**: HIGH (always fix lint before landing)

### 16. Rate-Limit Verbose Logging
**Check**: Rate-limit per-event logging (e.g., per-packet, per-request). Use `XLOG_EVERY_MS` or similar throttling instead of logging at line rate. Do not add an XLOG inside a tight loop or per-packet handler without rate limiting.
**Why**: Per-packet logging causes CPU hog and can destabilize the agent under load. One log line per 5 seconds is sufficient for debugging without impacting performance.
**Confidence**: HIGH (always rate-limit hot-path logging)

### 17. Perspective-Aware Naming
**Check**: When naming involves direction (Rx/Tx, send/receive, ingress/egress), make the reference frame explicit. Use `fromX`/`toX` naming (e.g., `fromSidebandAgent`, `toSidebandAgent`) instead of ambiguous `Rx`/`Tx` which can be interpreted from either the switch or the external agent perspective.
**Why**: Rx and Tx are ambiguous — readers default to the switch perspective, but the code may mean the opposite. Explicit directional naming eliminates confusion.
**Confidence**: MEDIUM (judgment call — Rx/Tx is fine when the perspective is unambiguous)

### 18. Update OSS Build Files Alongside Internal Changes
**Check**: When adding or moving source files, update OSS CMake files alongside BUCK targets. Also update the scope resolver when adding new state types or getters to SwitchState.
**Why**: OSS builds use CMake, not Buck. Forgetting to update CMake files breaks the open-source build, which is caught only after landing and delays vendor collaboration.
**Confidence**: HIGH (always update OSS CMake when modifying BUCK targets)

### 19. Empty Vector Over optional\<vector\>
**Check**: Use `vector<T>` (empty = no values) instead of `optional<vector<T>>` which is ambiguous
**Why**: `optional<vector>` conflates "no answer" with "empty answer". Callers can check `.empty()`.
**Source**: D59879844 reviewer feedback
**Confidence**: HIGH

### 20. Map-Based Config Over If-Chains
**Check**: Replace platform-specific if-else chains with `std::unordered_map<string, T>` lookups
**Why**: Adding a new platform requires only a map entry, not a code change. Reduces error-prone branching.
**Source**: D93164680 reviewer feedback
**Confidence**: MEDIUM

### 21. Include Both ID and Name in Logs
**Check**: When logging switchId, portId, or similar identifiers, include both numeric ID and human-readable name
**Why**: Names are better mnemonics for debugging. Numeric-only logs require cross-referencing.
**Source**: D61140726 reviewer feedback
**Confidence**: MEDIUM

### 22. No Hardcoded Magic Numbers When Enums/Constants Exist
**Check**: Never use hardcoded numeric literals when a named enum, macro, or constant exists for that value
**Why**: Magic numbers hide intent and break when enum values change. Existing usage of the same magic number is not a valid precedent.
**Source**: D100107691 reviewer feedback
**Confidence**: HIGH
