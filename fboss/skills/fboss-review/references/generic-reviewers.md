# Generic Reviewer Personas

## Reviewer 1: Reliability

Focus: Error handling, resource management, logging, timeout/retry, graceful degradation.

Review for:
- Missing error handling on fallible operations (SAI calls, Thrift RPCs, file I/O)
- Resource leaks: RAII violations, missing cleanup in error paths
- Missing or insufficient logging at error boundaries
- Timeout/retry logic: missing timeouts, unbounded retries, no backoff
- Graceful degradation: does failure in a non-critical path crash the process?

Severity guide: Resource leaks and missing error handling = HIGH. Logging gaps = MEDIUM.

## Reviewer 2: Engineering / Performance

Focus: Algorithmic complexity, unnecessary copies, lock contention, modern C++17/20, const/constexpr.

Review for:
- O(n^2) or worse algorithms where O(n) or O(n log n) is possible
- Unnecessary copies of large objects (use const ref or move semantics)
- Lock contention: holding locks during I/O, broad lock scopes
- Modern C++ opportunities: structured bindings, std::optional, constexpr
- Unnecessary allocations in hot paths
- Library dependency growth: does adding this dependency risk build time bloat for downstream consumers?

Severity guide: Algorithmic issues in hot paths = HIGH. Style modernization = LOW. Library dependency bloat = MEDIUM.

## Reviewer 3: Code Quality

Focus: Readability, modularity, duplication, API design. Also loads `../fboss-code-standards/references/general-patterns.md` for team conventions.

Review for:
- Code duplication that should be extracted
- Functions doing too many things (> 50 lines warrants scrutiny)
- Unclear variable/function names
- API design: are interfaces minimal, consistent, and hard to misuse?
- Team coding conventions from general-patterns.md
- Hardcoded magic numbers when named enums/constants exist

Severity guide: Duplication causing maintenance burden = MEDIUM. Naming = LOW. Magic numbers = MEDIUM.

## Reviewer 4: Summary & Test Plan

Focus: Diff metadata quality.

Review for:
- Title accuracy: does it describe what changed?
- Summary completeness: does it explain WHY, not just WHAT? Are design decisions documented?
- Test plan adequacy: are tests listed? Do they cover the change?
- Diff coherence: is this one logical change, or should it be split?

Severity guide: Missing test plan = HIGH. Title/summary improvements = LOW.

## Reviewer 5: Silent Failure Finder

Focus: Logic errors, lossy conversions, race conditions, silent data loss.

Review for:
- Off-by-one errors in loops and bounds checks
- Wrong comparison operators (< vs <=, == vs !=)
- Lossy numeric conversions (int64 -> int32, double -> int) without bounds checks
- Race conditions: shared mutable state accessed without synchronization
- Silent data loss: ignored return values, unchecked optional access
- Method override signature mismatches: any child class overriding a parent method must preserve the parent's full signature and forward all arguments. Read the parent class to verify — silently dropped arguments cause hard-to-detect data loss.

Severity guide: Logic errors and race conditions = HIGH. Lossy conversions = MEDIUM.
