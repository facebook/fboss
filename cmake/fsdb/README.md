# FSDB CMake Build System

This directory contains the CMake build files for FSDB (FBOSS State DataBase) components.

## Auto-Discovery of Services/Executables

The FSDB build system uses an **automatic discovery pattern** to ensure all FSDB services/executables are built together in CI/CD pipelines.

### How It Works

1. **Registration**: Each CMake file that defines an executable registers it by appending to the `FSDB_EXECUTABLES` list:
   ```cmake
   add_executable(my_executable ...)
   target_link_libraries(my_executable ...)

   # Register this executable for fsdb_all_services target
   set(FSDB_EXECUTABLES ${FSDB_EXECUTABLES} my_executable CACHE INTERNAL "List of all FSDB executables")
   ```

2. **Collection**: The `ZZZFsdbAllExecutables.cmake` file (named with ZZZ prefix to ensure it loads last alphabetically) creates a target that depends on all registered executables.

3. **Usage**: The `fsdb_all_services` target can be used to build all FSDB services/executables at once.

### Adding New Executables

When you add a new executable to FSDB, simply add this line after defining your executable:

```cmake
# Register this executable for fsdb_all_services target
set(FSDB_EXECUTABLES ${FSDB_EXECUTABLES} your_executable_name CACHE INTERNAL "List of all FSDB executables")
```

**That's it!** Your executable will automatically be included in:
- The `fsdb_all_services` target
- GitHub Actions builds
- Conveyor builds
- The `fboss_other_services` target

### Currently Registered Executables

As of now, the following executables are registered:
- `fsdb` - Main FSDB server binary (FsdbServer.cmake)
- `fsdb_utils_benchmark` - Benchmark utility (FsdbTests.cmake)
- `fsdb_pub_sub_tests` - Pub/Sub tests (FsdbTests.cmake)
- `fsdb_client_test` - Client tests (FsdbClientTest.cmake)

### Testing

On diff runs will now include your new executable and similar to D86990683, you can rely on oss-fboss-linux-getdeps-fboss_other_services job to test your executable.

### File Naming Convention

Note: `ZZZFsdbAllExecutables.cmake` is prefixed with `ZZZ` to ensure it's loaded **last** in alphabetical order. This is critical because it needs to access the `FSDB_EXECUTABLES` list after all other files have populated it.
