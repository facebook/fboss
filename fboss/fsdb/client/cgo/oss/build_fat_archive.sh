#!/bin/bash
# Build a fat static archive (libfsdb_cgo_bundle.a) from an FBOSS OSS CMake
# build directory + getdeps installed/ tree. The fat archive bundles the
# FsdbCgoPubSubWrapper and every transitive C++ dep (Folly, Thrift, fb303,
# glog, fmt, etc.) into a single .a so vendors can link with one -l flag.
#
# Usage:
#   ./build_fat_archive.sh <oss_build_dir> [<output_dir>]
#
# Output:
#   <output_dir>/libfsdb_cgo_bundle.a
#   <output_dir>/fsdb_cgo_api.h        (copied alongside)
#   <output_dir>/MANIFEST.txt          (size + sha256 + symbol count)
#
# Notes for vendors:
#   The bundle is a closed *static* link unit for FBOSS code. A few system
#   shared libs are still required at link time. For an FBOSS-OSS-built
#   bundle, vendors should add to their LDFLAGS (in addition to -lfsdb_cgo_bundle):
#
#     -lstdc++ -lm -lpthread -ldl -lssl -lcrypto -l:libz.so.1
#     -lgflags -lglog -l:libunwind.so.8
#     -l:libdouble-conversion.so.3 -l:libbz2.so.1 -l:libsnappy.so.1
#     -l:liblz4.so.1 -l:liblzma.so.5 -lxxhash
#
#   gflags is intentionally excluded from the bundle because libglog.so links
#   it dynamically; bundling it as static causes a "flag defined more than
#   once" runtime error from the double registration. Link gflags dynamically.
#
#   The bundle is compiled with clang and the libstdc++ new ABI (cxx11). Vendors
#   must link with a compatible toolchain (clang or recent gcc with libstdc++).
#
# Exit codes:
#   0  success
#   1  bad arguments / build dir doesn't exist
#   2  no .a files found under the build dir
#   3  symbol verification failed (one or more entry points missing)

set -euo pipefail

# The 19 entry points that MUST be present in the bundled archive.
EXPECTED_SYMBOLS=(
    FsdbInit
    FsdbCgoAbiVersion
    FsdbSetFlag
    CreateFsdbWrapper
    DestroyFsdbWrapper
    ShutdownFsdbWrapper
    SubscribeToPortMaps
    SubscribeToStatsPath
    SubscribeToStatePath
    HasStateSubscription
    HasStatsSubscription
    HasStatePathSubscription
    GetClientId
    WaitForStateUpdates
    WaitForStatsUpdates
    WaitForStatePathUpdates
    FreeStateUpdates
    FreeStatsUpdates
    FreeStatePathUpdates
)

# Patterns for archives to EXCLUDE from the bundle. `find` -name globs.
# - libfsdb_cgo_bundle.a:   self-ingest from prior runs (this script's output)
# - libgflags*.a:           dynamically linked via libglog.so → libgflags.so;
#                           bundling as static causes runtime double-registration
EXCLUDE_PATTERNS=(
    "libfsdb_cgo_bundle.a"
    "libgflags.a"
    "libgflags_nothreads.a"
)

# getdeps install dirs whose lib64/*.so* should be copied into runtime_libs/.
# These deps have C++ ABIs that often skew with what's preinstalled on vendor
# hosts (e.g. RHEL 9 ships glog 0.3.5 but getdeps builds 0.5.x). Shipping the
# matching .so files alongside the binary lets vendors run with
# `LD_LIBRARY_PATH=./runtime_libs` instead of patching their host's libs.
# Naming convention is the getdeps install dir prefix; suffix is a hash.
RUNTIME_LIB_DEPS=(
    "glog-"
    "libunwind-"
    "double-conversion-"
    "fmt-"
)

usage() {
    sed -n '2,/^$/p' "$0" | sed 's/^# \?//'
    exit 1
}

[[ $# -ge 1 ]] || usage
build_dir="$1"
output_dir="${2:-$(pwd)/out}"

[[ -d "$build_dir" ]] || { echo "ERROR: build dir not found: $build_dir" >&2; exit 1; }

# Resolve to absolute paths up front.
build_dir=$(cd "$build_dir" && pwd)
mkdir -p "$output_dir"
output_dir=$(cd "$output_dir" && pwd)
# Self-ingest of prior runs is prevented by EXCLUDE_PATTERNS below
# (libfsdb_cgo_bundle.a is filtered out of the find), so output_dir under
# build_dir is fine — the BZL OSS CI step relies on that layout.

# Locate the shim header so we can ship it next to the archive.
shim_header="$(dirname "$0")/../fsdb_cgo_api.h"
[[ -f "$shim_header" ]] || {
    echo "ERROR: shim header not found at $shim_header" >&2
    exit 1
}
shim_header=$(cd "$(dirname "$shim_header")" && pwd)/fsdb_cgo_api.h

echo "==> Scanning $build_dir for .a files"
# Build the find exclusion expression: -not -name pat1 -not -name pat2 ...
find_args=( "$build_dir" -type f -name '*.a' )
for pat in "${EXCLUDE_PATTERNS[@]}"; do
    find_args+=( -not -name "$pat" )
done
mapfile -t archives < <(find "${find_args[@]}" | sort)
[[ ${#archives[@]} -gt 0 ]] || {
    echo "ERROR: no .a files found under $build_dir" >&2
    exit 2
}
echo "    Found ${#archives[@]} archives (excluded: ${EXCLUDE_PATTERNS[*]})"

bundle="$output_dir/libfsdb_cgo_bundle.a"
echo "==> Bundling into $bundle (via ar MRI script)"
rm -f "$bundle"
# Use ar MRI script mode (`addlib`) to merge .a files directly without an
# extract+rebundle step. This preserves duplicate-named members (libfmt's
# format.cc.o, libthrift_service_client's two ThriftServiceClient.cpp.o, etc.)
# that the prior extract-based approach silently dropped — losing flag
# definitions like FLAGS_wedge_agent_port and fmt's vformat in the process.
mri_script=$(mktemp -t fsdb_cgo_bundle_mri.XXXXXX)
trap 'rm -f "$mri_script"' EXIT
{
    echo "create $bundle"
    for a in "${archives[@]}"; do
        echo "addlib $a"
    done
    echo "save"
    echo "end"
} > "$mri_script"
ar -M < "$mri_script"
total_members=$(ar t "$bundle" | wc -l)
echo "    Bundled $total_members object members from ${#archives[@]} archives"

echo "==> Verifying entry-point symbols are present"
missing=()
nm_output=$(nm --defined-only "$bundle" 2>/dev/null || true)
for sym in "${EXPECTED_SYMBOLS[@]}"; do
    if ! echo "$nm_output" | awk -v s="$sym" '$2 == "T" && $3 == s {found=1} END {exit !found}'; then
        missing+=("$sym")
    fi
done
if [[ ${#missing[@]} -gt 0 ]]; then
    echo "ERROR: missing entry points in bundle:" >&2
    printf '  %s\n' "${missing[@]}" >&2
    exit 3
fi
echo "    All ${#EXPECTED_SYMBOLS[@]} entry points present"

# Note: previous versions of this script ran `objcopy --localize-hidden` here as
# a "belt-and-braces" measure over -fvisibility=hidden on the wrapper TU. This
# was removed because third-party deps (libfmt in particular) are built with
# -fvisibility=hidden too, so localizing across the whole bundle silently
# converts fmt's vformat (and other transitive symbols vendors need at link
# time) from external to local. The wrapper TU's own -fvisibility=hidden in
# its BUCK target already keeps internal symbols hidden in the .o files; that
# visibility carries through `ar -M addlib` unchanged.

echo "==> Copying shim header"
cp "$shim_header" "$output_dir/fsdb_cgo_api.h"

echo "==> Collecting runtime shared libs"
runtime_dir="$output_dir/runtime_libs"
mkdir -p "$runtime_dir"
for dep_glob in "${RUNTIME_LIB_DEPS[@]}"; do
    mapfile -t sos < <(find "$build_dir/../installed/" "$build_dir/installed/" -maxdepth 4 -path "*${dep_glob}*/lib64/*.so*" 2>/dev/null | sort -u)
    if [[ ${#sos[@]} -gt 0 ]]; then
        for so in "${sos[@]}"; do
            cp -P "$so" "$runtime_dir/"
        done
        echo "    Copied ${#sos[@]} files matching ${dep_glob}*"
    else
        echo "    WARNING: no .so files found for ${dep_glob}*" >&2
    fi
done
# libglog 0.5+ has DT_NEEDED libunwind.so.1 (old soname); modern hosts ship .so.8.
if [[ -f "$runtime_dir/libunwind.so.8" ]] && [[ ! -e "$runtime_dir/libunwind.so.1" ]]; then
    ln -sf libunwind.so.8 "$runtime_dir/libunwind.so.1"
    echo "    Created libunwind.so.1 -> libunwind.so.8 shim"
fi

echo "==> Writing MANIFEST.txt"
size=$(stat -c '%s' "$bundle")
sha=$(sha256sum "$bundle" | awk '{print $1}')
exported=$(nm --defined-only "$bundle" 2>/dev/null | awk '$2 == "T"' | wc -l)
{
    echo "libfsdb_cgo_bundle.a"
    echo "  size:           $size bytes"
    echo "  sha256:         $sha"
    echo "  exported T:     $exported  (the ${#EXPECTED_SYMBOLS[@]} FSDB_CGO_API entry points + transitive deps required at link time)"
    echo "  source build:   $build_dir"
    echo "  archives merged: ${#archives[@]}"
    echo "  object members: $total_members"
    echo "  built at:       $(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$output_dir/MANIFEST.txt"

echo
echo "=== DONE ==="
echo "  $bundle  (${size} bytes, sha256=${sha:0:16}…)"
echo "  $output_dir/fsdb_cgo_api.h"
echo "  $output_dir/MANIFEST.txt"
