---
description: Check FBOSS code changes for OSS/GitHub build compatibility issues
oncalls:
  - fboss_oss
apply_to_regex: fbcode/fboss/(?!.*(facebook|fb)/).*\.(cpp|h|cc|hpp)$
apply_to_content: .
apply_to_clients: ['code_review']
---

# FBOSS OSS Compatibility Lints

---

## [OSS-001] Unguarded Internal Header Include
**Severity:** warning
**Description:** `#include` directives referencing Meta-internal paths that are not available in OSS builds. These MUST be guarded with `#ifndef IS_OSS` or replaced with OSS-compatible equivalents.

**Detection:** Flag any `#include` directive containing these paths:
- `common/network/`
- `neteng/`
- `servicerouter/`
- `configerator/`
- `tupperware/`
- `fb303/`
- `scribe/`

**Bad Example:**
```cpp
#include "common/network/AddressUtil.h"  // ❌ Not available in OSS
#include "configerator/structs/neteng/fboss/config.h"  // ❌ Not available in OSS
#include "servicerouter/client/cpp2/ServiceRouter.h"  // ❌ Not available in OSS
```

**Good Example:**
```cpp
#include "fboss/agent/AddressUtil.h"  // ✅ OSS-compatible
// OR guard internal includes:
#ifndef IS_OSS
#include "common/network/AddressUtil.h"
#endif
```

---

## [OSS-002] Unguarded Internal API Usage
**Severity:** warning
**Description:** Code calling Meta-internal APIs without `#ifndef IS_OSS` guards. Internal APIs are not available in OSS builds and will cause compilation failures.

**Detection:** Flag any usage of these API namespaces/functions without IS_OSS guards:
- `netwhoami::`
- `ServiceRouter::`
- `configerator::`
- `scribe::`
- `facebook::fb303::`
- `facebook::memcache::`

**Bad Example:**
```cpp
auto hostname = netwhoami::getHostname();  // ❌ Not available in OSS
auto sr = ServiceRouter::getInstance();  // ❌ Not available in OSS
scribe::log("fboss", msg);  // ❌ Not available in OSS
```

**Good Example:**
```cpp
#ifndef IS_OSS
auto hostname = netwhoami::getHostname();
#else
auto hostname = folly::getHostname();
#endif
```

---

## [OSS-003] New .cpp File Without CMake Update
**Severity:** warning
**Description:** Adding a new .cpp file requires updating the corresponding CMake file in `fboss/github/cmake/` for OSS builds.

**Detection:** If the diff adds a new `.cpp` file in `fboss/` (outside `facebook/` subdirs), check that the diff also modifies a CMake file in `fboss/github/cmake/`.

**CMake file mapping:**
- `fboss/agent/` → `Agent*.cmake`
- `fboss/cli/fboss2/` → `CliFboss2.cmake`
- `fboss/fsdb/` → `Fsdb*.cmake`
- `fboss/platform/` → `Platform*.cmake`
- `fboss/qsfp_service/` → `QsfpService*.cmake`

---

## Files Exempt from OSS Checks

Do NOT flag issues in:
- Files in `facebook/` subdirectories
- Files in `fb/` subdirectories

**Note:** Files in `test/` or `tests/` directories ARE included in OSS and MUST be checked.

---

## Quick Reference

| Lint ID | Severity | Pattern to Flag |
|---------|----------|-----------------|
| OSS-001 | warning | `#include "common/network/..."`, `#include "configerator/..."`, etc. |
| OSS-002 | warning | `netwhoami::`, `ServiceRouter::`, `scribe::`, etc. |
| OSS-003 | warning | New .cpp file without CMake update |
