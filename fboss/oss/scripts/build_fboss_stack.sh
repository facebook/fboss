#!/usr/bin/env bash
#
# Build script for FBOSS forwarding and platform stacks
# This script builds the specified stack binaries inside a container.
#
# Prerequisites (handled by build_entrypoint.py):
#   - SAI SDK extracted to /opt/sdk (from /deps/sai tarball)
#   - Kernel RPMs installed (from /deps/kernel tarball, optional)
#
# This script:
#   - Parses the requested stack type (forwarding or platform)
#   - For forwarding, enables SAI/SDK handling via need_sai=1
#   - Detects SAI location at /opt/sdk (when need_sai=1)
#   - Configures SAI environment variables (when need_sai=1)
#   - Builds FBOSS dependencies
#   - Builds the appropriate FBOSS CMake target
#   - Packages artifacts into tarballs
#
set -euxo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 forwarding|platform" >&2
  exit 1
fi

stack_type="$1"

need_sai=0
stack_label=""
cmake_target=""
package_target=""

case "$stack_type" in
forwarding)
  need_sai=1
  stack_label="forwarding"
  cmake_target="fboss_forwarding_stack"
  package_target="forwarding-stack"
  ;;
platform)
  stack_label="platform"
  cmake_target="fboss_platform_services"
  package_target="platform-stack"
  ;;
*)
  echo "Unsupported stack type: $stack_type (only 'forwarding' and 'platform' are supported)" >&2
  exit 1
  ;;
esac

OUT_DIR=/output

BUILD_TYPE="${BUILD_TYPE:-MinSizeRel}"

# Setup FBOSS build environment compatibility:
#    build_entrypoint provides: /src (repo), /deps/*-extracted (dependencies)
#    FBOSS build expects: /var/FBOSS/fboss (repo), /opt/sdk (SAI)
if [ -d "/src" ]; then
  echo "Setting up symlinks for FBOSS build environment"

  # Link /var/FBOSS/fboss -> /src (the worktree root)
  mkdir -p /var/FBOSS
  rm -rf /var/FBOSS/fboss # Remove any stale symlink or directory
  ln -sf /src /var/FBOSS/fboss
  echo "  Created: /var/FBOSS/fboss -> /src"

  if [ "$need_sai" -eq 1 ]; then
    # Link /opt/sdk -> extracted SAI dependency
    # build_entrypoint extracts /deps/sai to /deps/sai-extracted
    if [ ! -e "/opt/sdk" ]; then
      ln -sf /deps/sai-extracted /opt/sdk
      echo "  Created: /opt/sdk -> /deps/sai-extracted"
    fi
  fi
fi

if [ "$need_sai" -eq 1 ]; then
  # Look for SAI installation at /opt/sdk
  if [ -d "/opt/sdk" ]; then
    SAI_DIR="/opt/sdk"
  else
    echo "ERROR: No SAI found at /opt/sdk"
    exit 1
  fi
  echo "Found SAI at $SAI_DIR (installed by build_entrypoint.py)"

  # Source SAI build environment
  source "$SAI_DIR/sai_build.env"
  echo "SAI environment loaded from $SAI_DIR/sai_build.env"

  echo "Using SAI_SDK_VERSION=${SAI_SDK_VERSION:-N/A} for SAI_VERSION=${SAI_VERSION:-Unknown}"

  if [ -n "${BUILD_SAI_FAKE:-}" ]; then
    sai_name="sai-fake"
  elif [ -n "${SAI_BRCM_IMPL:-}" ]; then
    sai_name="sai-bcm-${SAI_SDK_VERSION}"
  else
    sai_name="sai-unknown-${SAI_SDK_VERSION}"
  fi
  echo "Using SAI implementation: $sai_name"
fi

# Use a scratch path for the CMake build tree.
# For forwarding we use /build (host-backed via distro_cli) so caches persist
# across container runs
scratch_root="/build"
if [ "$need_sai" -eq 1 ]; then
  common_root="${scratch_root}/forwarding-stack/common"
  build_dir="${scratch_root}/forwarding-stack/${sai_name}"
else
  common_root="${scratch_root}/platform-stack/common"
  build_dir="${scratch_root}/platform-stack"
fi
mkdir -p "$build_dir"

common_options='--allow-system-packages'
common_options+=' --scratch-path '$build_dir
common_options+=' --src-dir .'
common_options+=' fboss'

# Share download / repo / extracted caches across different types of builds
mkdir -p "${common_root}/downloads"
if [ ! -L "${build_dir}/downloads" ]; then
  ln -s "${common_root}/downloads" "${build_dir}/downloads"
fi
mkdir -p "${common_root}/repos"
if [ ! -L "${build_dir}/repos" ]; then
  ln -s "${common_root}/repos" "${build_dir}/repos"
fi
mkdir -p "${common_root}/extracted"
if [ ! -L "${build_dir}/extracted" ]; then
  ln -s "${common_root}/extracted" "${build_dir}/extracted"
fi

echo "Building FBOSS ${stack_label} stack"
echo "Output directory: $OUT_DIR"

# Navigate to FBOSS source root
cd /var/FBOSS/fboss

# Save the manifests because we must modify them, at a minimum to use the stable dependency hashes.
tar -cf manifests_snapshot.tar build
tar -xf fboss/oss/stable_commits/latest_stable_hashes.tar.gz

if [ "$stack_type" = "forwarding" ]; then
  # Setup SAI implementation
  SAI_INCLUDE_PATH="$SAI_DIR/include"
  echo "Using SAI include path for build-helper: $SAI_INCLUDE_PATH"

  SAI_IMPL_OUTPUT_DIR="$build_dir/sai_impl_output"
  echo "Preparing SAI manifests and HTTP server via build-helper.py into $SAI_IMPL_OUTPUT_DIR"

  # This will:
  #   * Copy libsai_impl.a and headers into $SAI_IMPL_OUTPUT_DIR
  #   * Create libsai_impl.tar.gz
  #   * Rewrite libsai and sai_impl manifests
  #   * Ensure fboss manifest depends on sai_impl
  #   * Start a local http.server on port 8000 serving libsai_impl.tar.gz
  #
  # getdeps.py will then be able to download sai_impl from
  # http://localhost:8000/libsai_impl.tar.gz using these manifests
  if [ -z "${BUILD_SAI_FAKE:-}" ]; then
    ./fboss/oss/scripts/build-helper.py \
      "$SAI_DIR/lib/libsai_impl.a" \
      "$SAI_INCLUDE_PATH" \
      "$SAI_IMPL_OUTPUT_DIR" \
      "$SAI_VERSION"
  else
    echo "BUILD_SAI_FAKE is set; skipping build-helper.py (no vendor SAI manifests)"
  fi
elif [ "$stack_type" = "platform" ]; then
  # For a platform-only build we do not need the vendor SAI implementation.
  # Temporarily drop sai_impl from the fboss manifest so getdeps will not try
  # to fetch it. The open-source SAI headers (libsai) remain in the manifest.
  if grep -q '^[[:space:]]*sai_impl[[:space:]]*$' build/fbcode_builder/manifests/fboss; then
    tmp_manifest="$(mktemp)"
    sed '/^[[:space:]]*sai_impl[[:space:]]*$/d' \
      build/fbcode_builder/manifests/fboss >"$tmp_manifest"
    mv "$tmp_manifest" build/fbcode_builder/manifests/fboss
    echo "Temporarily removed sai_impl from fboss manifest for platform-only build"
  fi
fi

# Install system dependencies
echo "Installing system dependencies..."
time nice -n 10 ./fboss/oss/scripts/run-getdeps.py install-system-deps \
  --recursive \
  ${common_options}

# Build dependencies
echo "Building FBOSS dependencies..."
time nice -n 10 ./fboss/oss/scripts/run-getdeps.py build \
  --only-deps \
  ${common_options}

echo "Get deps SUCCESS"

# Build FBOSS stack
echo "Building FBOSS ${stack_label} stack..."

time nice -n 10 ./fboss/oss/scripts/run-getdeps.py build \
  --no-deps \
  --build-type "${BUILD_TYPE:-MinSizeRel}" \
  --cmake-target "$cmake_target" \
  ${common_options}

echo "${cmake_target} Build SUCCESS"

# Package the stack
# Note: package.py creates both <target>.tar (production binaries)
# and <target>-tests.tar (test binaries). We only ship the production tar.
echo "Packaging ${stack_label} stack..."
python3 /var/FBOSS/fboss/fboss/oss/scripts/package.py \
  --build-dir "$build_dir" \
  "$package_target"

# Copy production artifact to output directory
# Tests are NOT included in production image
mkdir -p "$OUT_DIR"
mv "${package_target}.tar" "$OUT_DIR/"

echo "FBOSS ${stack_label} stack build complete!"
echo "Production artifact:"
ls -lh "$OUT_DIR"/"${package_target}.tar"

# Restore modified manifests if we took a snapshot earlier.
if [ -f manifests_snapshot.tar ]; then
  tar -xf manifests_snapshot.tar
  rm manifests_snapshot.tar
fi
