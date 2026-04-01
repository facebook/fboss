---
id: build_with_bazel
title: Building FBOSS with Bazel
sidebar_position: 2
description: How to build FBOSS using Bazel, with BUILD files auto-generated from BUCK
keywords:
    - FBOSS
    - OSS
    - build
    - bazel
oncall: fboss_oss
---

FBOSS has an experimental Bazel build that auto-generates BUILD files from the
existing Meta-internal BUCK files. This keeps the OSS build in sync with the
internal build mechanically, avoiding the manual divergence problems that
plagued the previous cmake-based OSS build.

## Prerequisites

The Bazel build runs inside the same Docker container used for the cmake build.
Follow the instructions in [Building FBOSS on Docker
Containers](building_fboss_on_docker_containers) to set up the container, then
install Bazel inside it:

```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install java-11-openjdk-devel zlib-devel
curl -L https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 -o bazelisk
chmod +x bazelisk
sudo mv bazelisk /usr/local/bin/bazel
```

## Generating BUILD Files

The BUILD files are auto-generated from BUCK files and are **never committed to
git**. Run the converter script to generate them:

```bash
python3 fboss/oss/scripts/bazelify.py
```

By default the script generates `getdeps_repository()` rules for each
third-party dependency. Bazel then builds (or downloads from an HTTP cache)
each dependency on demand when first needed — **no separate
`nhfboss-get-deps.sh` step is required.**

The script writes:

- `MODULE.bazel` -- Bazel module definition with third-party dep declarations
- `WORKSPACE.bazel` -- Empty file required by Bazel
- One `BUILD.bazel` per BUCK file directory (e.g., `fboss/agent/BUILD.bazel`)

The root `BUILD.bazel` (used by the parent nh.git repo) is left untouched.

### Converter Options

```bash
# Standard mode (on-demand dep builds via getdeps_repository rules) -- this is the default
python3 fboss/oss/scripts/bazelify.py

# Only regenerate if inputs changed (used by bazel.sh; ~100ms no-op)
python3 fboss/oss/scripts/bazelify.py --if-needed

# Dry run -- show what would be generated without writing files
python3 fboss/oss/scripts/bazelify.py --dry-run

# Annotate BUCK files with # bazelify: exclude for files not in cmake build
python3 fboss/oss/scripts/bazelify.py --generate-excludes

# Fall back to new_local_repository pointing at pre-built dirs (cmake build required first)
python3 fboss/oss/scripts/bazelify.py --no-getdeps-rules
```

> **Note:** If you use `bazel.sh` (recommended), you don't need to run
> `bazelify.py` manually — the wrapper calls `bazelify.py --if-needed`
> automatically before each build.

## Building Targets

Use `fboss/oss/scripts/bazel.sh` instead of bare `bazel`. The wrapper
script automatically:

1. **Regenerates Bazel BUILD files** if any input (BUCK, `.bzl`, manifests,
   patches) has changed since the last run. It calls `bazelify.py --if-needed`,
   which computes a SHA-256 checksum over all input files and compares it to
   the checksum stored in `MODULE.bazel` — if they match, regeneration is
   skipped (~100ms overhead). This means you never need to manually run
   `bazelify.py` after editing BUCK or `.bzl` files.
2. **Sets up sccache** for distributed compilation (idempotent — if sccache is
   already running it returns immediately).
3. **Assembles `.bazel.d/` fragments** into `.bazelrc.d` for Bazel to import.

Then it `exec`s the real Bazel binary. For convenience, add an alias to your
shell config:

```bash
alias bazel=/var/FBOSS/fboss/fboss/oss/scripts/bazel.sh
```

Then use Bazel normally:

```bash
# Build a specific library
bazel build //fboss/agent:switch_config

# Build a test binary
bazel build //fboss/cli/fboss2/test/config:cmd_config_test

# Run a test
bazel test //fboss/cli/fboss2/test/config:cmd_config_test

# Build all targets in a package (tolerating failures)
bazel build --keep_going //fboss/agent/...

# Build everything
bazel build --keep_going //fboss/...
```

Target names mirror the BUCK target names. For example, the BUCK target
`//fboss/agent:switch_config` becomes the Bazel target
`//fboss/agent:switch_config`.

### Compile Commands for Clang Tooling

Clang tools (clang-tidy, clang-include-cleaner, clangd) need a
`compile_commands.json` file describing how each source file is compiled.
Generate it with:

```bash
bazel run //fboss/oss/refresh:refresh_compile_commands
```

This uses
[hedronvision/bazel-compile-commands-extractor](https://github.com/hedronvision/bazel-compile-commands-extractor)
to walk Bazel's action graph and emit a correct `compile_commands.json`
covering all `//fboss/...` targets (including generated thrift files).
The output is written to
`/var/FBOSS/tmp_bld_dir/build/fboss/compile_commands.json` (same path as
the cmake build), so clang tooling pointed at that location works with
either build system. Set `FBOSS_COMPILE_COMMANDS_PATH` to override.
Re-run after adding new files or changing compile flags.

### Site-Specific Configuration

The build can be customized for your infrastructure via an environment file.
Copy the example and fill in your values:

```bash
cp fboss/oss/scripts/fboss-build.env.example fboss/oss/scripts/fboss-build.env
# Edit fboss-build.env with your infrastructure URLs
```

Available settings (all optional — leave empty to disable):

| Variable | Purpose |
|----------|---------|
| `GETDEPS_CACHE_URL` | HTTP cache for pre-built dependency tarballs (speeds up first build) |
| `SCCACHE_SCHEDULER_HOST` | Distributed sccache scheduler for remote compilation |
| `SCCACHE_AUTH_TOKEN` | Auth token for the sccache scheduler |
| `BAZEL_REMOTE_CACHE_URL` | [bazel-remote](https://github.com/buchgr/bazel-remote) cache server |

The `fboss-build.env` file is gitignored and will not be committed. Without
it, the build works normally — dependencies are built from source and
compilation runs locally (just slower).

### Remote Cache and Local Customization

Bazel configuration can be customized by adding `.bazelrc` fragments under
`.bazel.d/`. The `bazel.sh` wrapper assembles all `*.bazelrc` files in that
directory into `.bazelrc.d` before each build, which `.bazelrc` picks up via
`try-import`.

When `BAZEL_REMOTE_CACHE_URL` is set in `fboss-build.env`, `bazel.sh`
automatically generates `.bazel.d/remote-cache.bazelrc` with the appropriate
flags. With `--remote_local_fallback=true` the cache is silently skipped when
the server is unreachable. Bazel also maintains a local disk cache
automatically.

You can add any number of additional fragments under `.bazel.d/` for
site-specific or personal configuration without modifying the committed
`.bazelrc`.

## Current Limitations

The Bazel build is a work in progress. Known limitations:

- **Sandboxing is disabled.** FBOSS source files frequently include headers
  that are not declared in the BUILD files. Bazel's sandboxing is turned off
  via `--spawn_strategy=local` in `.bazelrc` until all headers are properly
  declared.

- **Some targets fail due to Meta-internal dependencies.** Targets that
  transitively depend on code under `facebook/` directories or on internal
  services (configerator, scribe, etc.) may fail to compile.

- **SAI and BCM SDK targets.** Targets that depend on vendor SAI or Broadcom
  SDK headers require additional setup (the SAI include path from the
  `libsai` package).

- **Not all thrift features are supported.** The thrift macro currently
  generates C++ (cpp2) code only. Thrift service stubs
  (`FbossCtrlAsyncClient.h`, etc.) are not yet generated.

- **Only real SAI builds are supported.** The Bazel build currently only works
  with a real SAI implementation (e.g., Broadcom SAI). Fake SAI builds and other
  build flavors are not yet supported. The SAI version and related defines
  (e.g., `SAI_VER_MAJOR`, `SAI_VER_MINOR`) are hardcoded in `.bazelrc` rather
  than being parameterized. `bazel.sh` checks for this and fails early with
  a helpful error if `sai_impl` is not configured.

---

## Design and Architecture

This section documents the design decisions, architecture, and inner workings
of the BUCK-to-Bazel conversion.

### Motivation

FBOSS has two parallel build systems:

- **BUCK** -- Used internally at Meta. This is the source of truth for target
  definitions, dependencies, and source file lists.
- **cmake** -- Used for the OSS build. Maintained by hand in `cmake/*.cmake`
  files and `CMakeLists.txt`.

The cmake build frequently diverges from BUCK because changes are made
manually. A tool (`fboss/oss/scripts/buck_cmake_dep_checker.py`) existed to
detect missing cmake dependencies, but the root cause -- manual synchronization
-- remained.

The Bazel build eliminates this by auto-generating BUILD files from BUCK files.
The generated files are never committed, so they always reflect the current
state of the BUCK files.

### Why Bazel (Not cmake)

- **Mechanical translation.** BUCK and Bazel share Starlark syntax and similar
  rule semantics (`cc_library`, `cc_test`, etc.), making the translation
  straightforward. cmake has fundamentally different syntax.
- **Dependency graph.** Bazel has a first-class dependency graph with per-target
  granularity, just like BUCK. cmake tends toward monolithic library targets.
- **Caching.** Bazel has built-in remote caching and remote execution,
  replacing the sccache setup used by the cmake build.
- **Reproducibility.** Bazel builds are hermetic by design (sandboxing, content
  hashing), reducing "works on my machine" problems.

### Architecture Overview

```
BUCK files (source of truth)
    |
    v
bazelify.py (converter script)
    |
    +---> MODULE.bazel (third-party deps via getdeps_repository rules)
    +---> BUILD.bazel files (one per BUCK file directory)
    |
    v
Bazel build (loading phase)
    |
    +---> getdeps_dep.bzl (repository rule per dep)
    |         |
    |         v
    |     cached-get-deps.py (downloads from HTTP cache or builds via getdeps)
    |
    +---> thrift_library.bzl (genrule + cc_library for .thrift files)
    +---> .bazelrc (clang toolchain, flags, sccache)
```

### The Converter Script

**File:** `fboss/oss/scripts/bazelify.py`

The converter has four main phases:

#### 1. Third-Party Dependency Discovery

By default, the script reads Meta's getdeps manifest files
(`build/fbcode_builder/manifests/`) to build a dependency graph of all
third-party libraries. Each library becomes a `getdeps_repository()` call in
`MODULE.bazel` (defined in `fboss/build_defs/getdeps_dep.bzl`).

When Bazel evaluates the repository rule, it invokes
`fboss/oss/scripts/cached-get-deps.py`, which:
1. Computes the getdeps project hash (a content hash of the manifest, revision,
   and all cmake defines)
2. Checks an HTTP cache (bazel-remote) for a matching pre-built tarball
3. On cache hit: downloads and extracts the tarball (typically a few seconds)
4. On cache miss: builds from source using `getdeps.py build --no-deps` and
   uploads the result to the cache

This means **third-party dependencies are built lazily by Bazel on first use**,
cached per-dependency between CI runs, and never require a manual
`nhfboss-get-deps.sh` step.

Each dep gets a generated `build_file_content` with:
- `cc_library` for static libraries (globs `lib/*.a` or `lib64/*.a`)
- Header-only `cc_library` for header-only packages (e.g., range-v3, CLI11)
- Special-cased targets for googletest, folly, boost (see `_make_build_file_content`)

Inter-dependencies between pre-built libraries are declared statically (e.g.,
folly depends on fmt, glog, gflags, boost).

#### 2. BUCK Parsing

The parser uses regex-based extraction (not a full Starlark parser) to find
target definitions in BUCK files. It handles:

- **Rule types:** `cpp_library`, `cpp_binary`, `cpp_unittest`, `cpp_benchmark`,
  `thrift_library`, `python_binary`, `python_library`, `export_file`
- **Attributes:** `name`, `srcs`, `headers`, `deps`, `exported_deps`,
  `external_deps`, `exported_external_deps`, `compiler_flags`,
  `propagated_pp_flags`, `thrift_srcs`, `thrift_cpp2_options`, `cpp2_deps`,
  `languages`, `link_whole`
- **External dep formats:** Simple strings (`"gflags"`), tuples
  (`("boost", None, "boost_container")`), and `fbsource//third-party/` paths
- **Annotations:** Special comments like `# bazelify: exclude` to control
  generation

Internal-only macros (loaded from `@fbcode_macros//` or non-existent `.bzl`
files) are detected and skipped.

#### 3. Dependency Resolution

Dependencies are resolved through a chain of mappings:

| BUCK Pattern | Bazel Label |
|---|---|
| `:name` | `:name` (same-package) |
| `//fboss/path:name` | `//fboss/path:name` (kept as-is) |
| `//fboss/path:name-cpp2-types` | `//fboss/path:name` (thrift suffix stripped) |
| `//folly/...`, `//thrift/...`, etc. | `@folly//:folly`, `@fbthrift//:fbthrift`, etc. |
| `fbsource//third-party/glog:glog` | `@glog//:glog` |
| `("boost", None, "boost_filesystem")` | `@boost//:boost` |
| `//configerator/...`, `//common/services/...` | Stripped (Meta-internal) |
| `//fboss/.../facebook/...` | Stripped (Meta-internal) |

Dependencies that reference repos not available in the getdeps install
directory are silently dropped.

#### 4. BUILD File Generation

Each BUCK rule type is translated to its Bazel equivalent:

| BUCK | Bazel | Notes |
|---|---|---|
| `cpp_library` | `cc_library` | `headers` becomes `hdrs`; `exported_deps` + `deps` both become `deps` |
| `cpp_binary` | `cc_binary` | |
| `cpp_unittest` | `cc_test` | `@googletest//:gtest_main` auto-added |
| `cpp_benchmark` | `cc_binary` | Treated as binary |
| `thrift_library` | `fboss_thrift_library` | Custom macro; only cpp2 generated |
| `export_file` | `exports_files` | |

**OSS file replacement.** When a source file like `CmdList.cpp` has a
corresponding `oss/CmdList.cpp`, the OSS version is used instead. This avoids
including Meta-internal headers that don't exist in the OSS repo.

**Header auto-discovery.** When a `cpp_library` has no explicit `headers`
attribute (common in FBOSS BUCK files), the converter checks for `.h` files
matching the `.cpp` files in `srcs` and adds them to `hdrs`.

**Subpackage filtering.** Source files that reference paths crossing into
subdirectories with their own BUCK file are filtered out, since Bazel does not
allow a target to include files from another package.

### Thrift Compilation

**File:** `fboss/build_defs/thrift_library.bzl`

The `fboss_thrift_library` macro compiles `.thrift` files to C++ in two steps:

1. A `genrule` runs the fbthrift compiler (`thrift1`) with `--gen mstch_cpp2`
   to produce generated C++ source and header files under a `gen-cpp2/`
   subdirectory.
2. A `cc_library` compiles the generated sources and links against the
   fbthrift and folly runtime libraries.

The thrift compiler and its include paths point to the pre-built fbthrift
installation from getdeps.

### Clang Toolchain

The build uses clang from `/usr/local/llvm/bin/`. This is configured in
`.bazelrc` via `--repo_env=CC` and `--repo_env=CXX`, which causes Bazel to
auto-detect and configure the clang toolchain.

Compiler flags are carried over from `CMakeLists.txt` (the clang-specific
`if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")` block) and from `run-getdeps.py`
(the additional flags for third-party dep compatibility).

### Why Repository Rules (Not genrules) for Third-Party Deps

Third-party deps are built by `getdeps_repository` **repository rules** rather
than `genrule` actions. This was a deliberate choice after evaluating the
alternatives.

**Why not genrules?** `genrule` requires all outputs to be enumerated in
`outs`. Third-party C++ deps produce hundreds of headers spread across nested
subdirectories — you cannot enumerate them statically, and `glob()` over genrule
outputs is impossible (glob runs at loading time; outputs don't exist until
execution time).

**Why not a custom Starlark rule with TreeArtifact outputs?** A custom
`cc_library`-like rule using `ctx.actions.declare_directory()` could produce
the install tree as a single opaque artifact cached by bazel-remote. This is
technically feasible but comes at significant cost: every `@depname//:depname`
label across all generated BUILD files and hand-written `.bzl` macros would
need to change to `//fboss/third_party/depname:depname`, and a correct
`CcInfo`-returning Starlark rule (headers, static libs, shared libs, alwayslink,
header-only deps, system linkopts) is a non-trivial amount of Starlark.

**Why not `http_archive` with pre-built tarballs?** This is the cleanest
long-term option — a separate publish-deps CI job would build all 56 deps and
upload versioned tarballs; `MODULE.bazel` would use standard `http_archive`
with `urls` + `sha256`, and Bazel's built-in `--repository_cache` would handle
downloads. The trade-off is a separate publish workflow and explicit SHA
management in `MODULE.bazel` whenever a dep's manifest or revision changes.
Worth revisiting once the cmake build is fully retired and the dep set
stabilizes.

**Current approach:** `repository_rule` with `local=True` + per-dep HTTP
caching in `cached-get-deps.py`. Repository rules run during Bazel's loading
phase, before compilation starts. Bazel re-runs a rule only when its declared
inputs (manifest, revision file, patches) change. The manual HTTP caching
handles the cross-VM and cross-CI-run caching that Bazel's remote cache does
not cover for repository rules. The ~60-line caching layer in
`cached-get-deps.py` is well-tested and simpler than the alternatives above.

### Bazel Module Configuration

The project uses Bazel 9's bzlmod system (`MODULE.bazel`) rather than the
legacy `WORKSPACE`. Third-party deps are declared as `getdeps_repository()`
repository rules (from `fboss/build_defs/getdeps_dep.bzl`). Bazel builds each
dep lazily during the loading phase via `cached-get-deps.py`.

The `disable_include_validation.patch` is applied to `rules_cc` via a
`single_version_override` patch. This disables Bazel's strict C++ header
include validation, which would otherwise fail because FBOSS BUCK files don't
declare all headers explicitly (complementing the `--spawn_strategy=local`
setting in `.bazelrc`).

The `MODULE.bazel` file is regenerated each time the converter runs, so it
automatically picks up new or updated dependencies and manifests.

### Hand-Written BUILD Files

A small number of BUILD files are hand-written for targets that exist in the
repo but don't have BUCK files:

- `common/network/if/BUILD.bazel` -- `Address.thrift` (network address types)
- `configerator/structs/neteng/fboss/thrift/BUILD.bazel` --
  `common.thrift` (configerator common types, synced from Meta's configerator
  repo)

### Annotation System

The converter supports special comments in BUCK files to control generation.

**File-level** (placed anywhere in the BUCK file as comments):

```python
# bazelify: exclude-all          -- Skip all C++ targets (thrift targets still generated)
# bazelify: include foo.bzl:func -- Bazel-native include (see below)
```

**Target-level** (placed on the lines immediately before a target definition):

```python
# bazelify: exclude              -- Skip this target
# bazelify: extra_dep = X       -- Add an extra dependency
# bazelify: add_src = oss/Foo.cpp -- Add an OSS source file to the target
```

**Inline** (placed at the end of a source file line inside a target):

```python
    "Foo.cpp",  # bazelify: exclude  -- Exclude this file from the target
```

Use `bazelify.py --generate-excludes` to systematically annotate BUCK files
by comparing against the cmake build. It identifies files, targets, and
entire BUCK files that are not part of the OSS build.

### Bazel-Native Include Mechanism

Some BUCK files use internal Meta macros (loaded from `.bzl` files that don't
exist in the OSS repo) to generate targets that can't be auto-translated by
`bazelify.py`. For these cases, you can write a hand-crafted `.bzl` file with
a function that creates the missing Bazel targets, and use the `include`
annotation to weave it into the generated `BUILD.bazel`:

```python
# In the BUCK file:
# bazelify: include sai_switch_impl.bzl:sai_switch_impl
```

This causes `bazelify.py` to emit the following in the generated
`BUILD.bazel`:

```python
load(":sai_switch_impl.bzl", "sai_switch_impl")

# ... auto-generated targets from BUCK ...

sai_switch_impl()
```

The `.bzl` file is committed to git and defines a function that calls
`native.glob()`, `cc_library()`, `cc_binary()`, etc. to create the targets:

```python
# sai_switch_impl.bzl
load("@rules_cc//cc:defs.bzl", "cc_library")

def sai_switch_impl():
    cc_library(
        name = "sai_switch",
        srcs = native.glob(["*.cpp"], exclude = ["facebook/*.cpp"]),
        hdrs = native.glob(["*.h"]),
        deps = ["//fboss/agent:core", "@folly//:folly"],
        visibility = ["//visibility:public"],
    )
```

**Key properties:**

- The `.bzl` file survives `bazelify.py` regeneration -- only the
  `BUILD.bazel` is regenerated, and it re-emits the `load()` + call.
- Multiple `include` annotations can be used in the same BUCK file.
- The `include` annotation can be combined with `exclude-all`: use
  `exclude-all` to suppress auto-generated C++ targets, and `include` to
  provide hand-written replacements via `.bzl` macros. Thrift targets are
  still auto-generated even with `exclude-all`, so the generated
  `BUILD.bazel` will contain both the thrift targets and the `.bzl` macro
  call. If a BUCK file uses `exclude-all` and all its targets are excluded
  (no thrift), the `BUILD.bazel` is still written as long as there is at
  least one `include` annotation.
- The `.bzl` function can define targets with the same name as what the
  internal macro would have generated, keeping the dependency graph intact
  for other packages that reference those targets.

**Current `.bzl` files** (for SAI/wedge_agent support):

| File | Targets | Purpose |
|---|---|---|
| `fboss/agent/hw/sai/api/sai_api_impl.bzl` | `sai_api`, `sai_traced_api` | SAI API + traced API wrapper |
| `fboss/agent/hw/sai/switch/sai_switch_impl.bzl` | `sai_switch`, `thrift_handler` | SAI switch manager |
| `fboss/agent/hw/sai/tracer/sai_tracer_impl.bzl` | `sai_tracer` | SAI API tracing |
| `fboss/agent/hw/sai/diag/sai_diag_impl.bzl` | `diag_lib` | SAI diagnostic shell |
| `fboss/agent/platforms/sai/sai_platform_impl.bzl` | `sai_platform`, `wedge_agent-sai_impl` | Platform library + agent binary |

### File Inventory

| File | Purpose | Committed? |
|---|---|---|
| `fboss/oss/scripts/bazelify.py` | Converter script | Yes |
| `fboss/oss/scripts/cached-get-deps.py` | Per-dep build/cache wrapper for getdeps | Yes |
| `fboss/build_defs/getdeps_dep.bzl` | `getdeps_repository` Bazel repository rule | Yes |
| `fboss/build_defs/disable_include_validation.patch` | Patch to disable rules_cc header validation | Yes |
| `fboss/build_defs/thrift_library.bzl` | Thrift compilation macro | Yes |
| `fboss/build_defs/BUILD.bazel` | Package marker | Yes |
| `.bazelrc` | Bazel configuration | Yes |
| `common/network/if/BUILD.bazel` | Hand-written thrift target | Yes |
| `configerator/.../BUILD.bazel` | Hand-written thrift target | Yes |
| `fboss/agent/hw/sai/*/sai_*_impl.bzl` | Hand-written SAI library macros | Yes |
| `fboss/agent/platforms/sai/sai_platform_impl.bzl` | SAI platform + wedge_agent | Yes |
| `fboss/agent/hw/sai/store/BUILD.bazel` | Hand-written SAI store library | Yes |
| `fboss/build/build_info.bzl` | SDK build info genrule macro | Yes |
| `MODULE.bazel` | Generated module config | No (generated) |
| `WORKSPACE.bazel` | Empty, required by Bazel | No (generated) |
| `fboss/*/BUILD.bazel` | Generated build targets | No (generated) |
