# Platform & Config Patterns

Patterns for code under `fboss/platform/`, platform services (sensor_service, fan_service), and configuration files.

### 1. JSON Config Schema Sync
**Check**: When modifying platform config, update both the JSON schema and the corresponding Thrift structure. Changes to one without the other cause deserialization failures.
**Why**: JSON configs are deserialized into Thrift structs. Schema mismatch causes config load failures at agent startup.
**Confidence**: HIGH (always a bug)

### 2. Per-Platform Testing
**Check**: Platform-specific code changes must include test coverage for affected platforms. Check that platform-specific test targets exist and are updated.
**Why**: Platform code often has subtle differences across hardware (Memory/memory, memory/memory, memory). Untested platforms may break silently.
**Confidence**: MEDIUM (depends on scope of change)

### 3. Startup Order Dependencies
**Check**: When adding new service dependencies during platform initialization, declare them explicitly in the startup sequence. Check for circular dependencies.
**Why**: Undeclared startup dependencies cause race conditions where services access uninitialized components.
**Confidence**: HIGH (causes crashes or undefined behavior)

### 4. Config Compile-Time Generation
**Check**: After modifying config templates or Thrift config structures, regenerate compiled configs using the appropriate build targets.
**Why**: Stale compiled configs cause runtime mismatches between expected and actual configuration.
**Confidence**: HIGH (always required after config schema changes)

### 5. OSS/Internal Split
**Check**: Changes to files that are open-sourced must also update the corresponding CMake build files. Check if the file is under an OSS-exported directory.
**Why**: Missing CMake updates break the open-source build, which is tested separately and may not fail until the OSS CI runs.
**Confidence**: MEDIUM (only applies to OSS-exported code)
