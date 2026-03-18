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

log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"
}

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 forwarding|platform" >&2
  exit 1
fi

stack_type="$1"

need_sai=0
stack_label=""
cmake_target=""
package_target=""
build_with_qsfp_service_sai="no"

case "$stack_type" in
forwarding)
  need_sai=1
  stack_label="forwarding"
  cmake_target="fboss_forwarding_stack"
  package_target="forwarding-stack"
  mem_per_core=16 # GB
  ;;
platform)
  stack_label="platform"
  cmake_target="fboss_platform_services"
  package_target="platform-stack"
  mem_per_core=7 # GB
  ;;
*)
  echo "Unsupported stack type: $stack_type (only 'forwarding' and 'platform' are supported)" >&2
  exit 1
  ;;
esac

gb_per_core=$(free -g | awk '/^Mem:/{print int($2 / '$mem_per_core')}')
num_cores=$(nproc)
if [ "$num_cores" -gt "$gb_per_core" ]; then
  num_jobs="$gb_per_core"
else
  num_jobs="$num_cores"
fi
export num_jobs # Export so it's available in subshells

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
    # build_entrypoint extracts /deps/hw_agent_sai to /deps/hw_agent_sai-extracted
    if [ ! -e "/opt/sdk" ]; then
      ln -sf /deps/hw_agent_sai-extracted /opt/sdk
      echo "  Created: /opt/sdk -> /deps/hw_agent_sai-extracted"
    fi

    if [ ! -e "/opt/qsfp-sdk" ]; then
      ln -sf /deps/qsfp_service_sai-extracted /opt/qsfp-sdk
      echo "  Created: /opt/qsfp-sdk -> /deps/qsfp_service_sai-extracted"
    fi
  fi
fi

SAI_DIR=""
if [ "$need_sai" -eq 1 ]; then
  # Look for SAI installation at /opt/sdk
  if [ -d "/opt/sdk" ]; then
    SAI_DIR="/opt/sdk"
  fi

  # Check if qsfp_service_sai build environment exists (will be sourced in subshell later)
  if [ -f "/opt/qsfp-sdk/qsfp_service_sai_build.env" ]; then
    SAI_DIR="/opt/qsfp-sdk"
    build_with_qsfp_service_sai="yes"
  fi
  echo "Found SAI at $SAI_DIR (installed by build_entrypoint.py)"
fi

# Function to perform complete build with given suffix, called in a subshell
# to isolate environments
perform_build() {
  local build_suffix="$1"
  local output_suffix="$2"
  local sai_env_file="$3"

  log "perform_build Command: $0, ARGS: $build_suffix $output_suffix $sai_env_file"

  # Source SAI build environment if provided
  if [ -n "$sai_env_file" ] && [ -f "$sai_env_file" ]; then
    # shellcheck disable=SC1090
    source "$sai_env_file"
    log "SAI environment loaded from $sai_env_file"
  fi

  # Determine SAI implementation name based on environment variables
  local sai_name=""
  if [ "$need_sai" -eq 1 ]; then
    log "Using SAI_SDK_VERSION=${SAI_SDK_VERSION:-N/A} for SAI_VERSION=${SAI_VERSION:-Unknown}"

    if [ -n "${BUILD_SAI_FAKE:-}" ]; then
      sai_name="sai-fake"
      export BUILD_SAI_FAKE
      export BUILD_SAI_FAKE_LINK_TEST
    elif [ -n "${SAI_BRCM_IMPL:-}" ]; then
      sai_name="sai-bcm-${SAI_SDK_VERSION}"
      export SAI_BRCM_IMPL
    elif [ -n "${SAI_BRCM_PAI_IMPL:-}" ]; then
      sai_name="sai-brcm-pai-${SAI_SDK_VERSION}"
      export SAI_BRCM_PAI_IMPL
    else
      sai_name="sai-unknown-${SAI_SDK_VERSION}"
    fi
    log "Using SAI implementation: $sai_name"
    if [ -n "${SAI_VERSION:-}" ]; then
      export SAI_VERSION
    fi
    if [ -n "${SAI_SDK_VERSION:-}" ]; then
      export SAI_SDK_VERSION
    fi
  fi

  scratch_root="/build"
  # Setup build directories based on scratch_root
  if [ "$need_sai" -eq 1 ]; then
    build_dir="${scratch_root}/forwarding-stack/${sai_name}${build_suffix}"
  else
    build_dir="${scratch_root}/platform-stack${build_suffix}"
  fi
  mkdir -p "$build_dir"

  common_root="${scratch_root}/common"

  common_options='--allow-system-packages'
  common_options+=' --scratch-path '$build_dir
  common_options+=' --src-dir .'
  common_options+=' --extra-cmake-defines {'
  common_options+='"CMAKE_BUILD_TYPE":"MinSizeRel"'
  common_options+=',"CMAKE_CXX_STANDARD":"20"'
  common_options+=',"RANGE_V3_TESTS":"OFF"'
  common_options+=',"RANGE_V3_PERF":"OFF"}'
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

  # Navigate to FBOSS source root
  cd /var/FBOSS/fboss

  log "Building FBOSS ${stack_label} stack${build_suffix}"

  # Save the manifests because we must modify them
  tar -cf manifests_snapshot.tar build
  tar -xf fboss/oss/stable_commits/latest_stable_hashes.tar.gz
  chmod -R a+r build/fbcode_builder/manifests

  # Setup SAI environment
  log "Setting up SAI environment"
  if [ "$stack_type" = "forwarding" ]; then
    # Setup SAI implementation
    SAI_INCLUDE_PATH="$SAI_DIR/include"
    log "Using SAI include path for build-helper: $SAI_INCLUDE_PATH"

    SAI_IMPL_OUTPUT_DIR="$build_dir/sai_impl_output"
    log "Preparing SAI manifests and HTTP server via build-helper.py into $SAI_IMPL_OUTPUT_DIR"

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
      log "BUILD_SAI_FAKE is set; skipping build-helper.py (no vendor SAI manifests)"
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
      log "Temporarily removed sai_impl from fboss manifest for platform-only build"
    fi
  fi

  # Install system dependencies
  log "Installing system dependencies..."
  time nice -n 10 ./fboss/oss/scripts/run-getdeps.py install-system-deps \
    --recursive \
    $common_options

  # Build dependencies
  log "Building FBOSS dependencies..."
  time nice -n 10 ./fboss/oss/scripts/run-getdeps.py build \
    --build-type $BUILD_TYPE \
    --only-deps \
    $common_options

  log "Get deps SUCCESS"

  # Build FBOSS stack
  log "Building FBOSS ${stack_label} stack..."

  time nice -n 10 ./fboss/oss/scripts/run-getdeps.py build \
    --num-jobs "${num_jobs}" \
    --build-type "${BUILD_TYPE}" \
    --no-deps \
    ${common_options} \
    --cmake-target "${cmake_target}"

  log "${cmake_target} Build SUCCESS"

  # Package the stack
  # Note: package.py creates both <target>.tar (production binaries)
  # and <target>-tests.tar (test binaries). We only ship the production tar.
  log "Packaging ${stack_label} stack..."
  python3 /var/FBOSS/fboss/fboss/oss/scripts/package.py \
    --build-dir "$build_dir" \
    "$package_target"

  # Copy production artifact to output directory
  # Tests are NOT included in production image
  log "Copying artifacts to output"
  OUT_DIR=/output
  mkdir -p "$OUT_DIR"
  mv "${package_target}.tar" "$OUT_DIR/${package_target}${output_suffix}.tar"

  log "FBOSS ${stack_label} stack build complete!"
  log "Production artifact:"
  ls -lh "$OUT_DIR/${package_target}${output_suffix}.tar"

  # Restore modified manifests if we took a snapshot earlier.
  if [ -f manifests_snapshot.tar ]; then
    tar -xf manifests_snapshot.tar
    rm manifests_snapshot.tar
  fi
}

# First build: standard build with current settings
if [ "$need_sai" -eq 1 ]; then
  (
    log "Building with hw_agent SAI settings"
    perform_build "" "" "$SAI_DIR/sai_build.env"
  )
  if [ "$build_with_qsfp_service_sai" = "yes" ]; then
    (
      log "Building with qsfp_service_sai settings"
      perform_build "-qsfp-sai" "-qsfp-sai" "/opt/qsfp-sdk/qsfp_service_sai_build.env"
    )
  fi
else
  (
    log "Building platform stack"
    perform_build "" "" ""
  )
fi
