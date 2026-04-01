#!/bin/bash
# Wrapper around bazel that handles build environment setup:
#
#   1. Sources site-specific configuration from fboss-build.env (if present).
#      See fboss-build.env.example for available settings.
#
#   2. Regenerates BUILD.bazel files from BUCK files when inputs change.
#
#   3. Sets up sccache for distributed compilation (idempotent).
#
#   4. Assembles .bazel.d/*.bazelrc fragments into .bazelrc.d (picked up by
#      .bazelrc via try-import). Add your own fragment under .bazel.d/ to
#      customize the build (e.g. remote cache URL, extra flags).
#
# Usage: same as bazel. Either invoke directly:
#   fboss/oss/scripts/bazel.sh build //fboss/...
#
# Or add an alias in your shell config:
#   alias bazel=/var/FBOSS/fboss/fboss/oss/scripts/bazel.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BAZEL_D="$REPO_ROOT/.bazel.d"
BAZELRC_D="$REPO_ROOT/.bazelrc.d"

# Source site-specific configuration if present.
ENV_FILE="$SCRIPT_DIR/fboss-build.env"
if [ -f "$ENV_FILE" ]; then
  # shellcheck source=/dev/null
  source "$ENV_FILE"
fi

# Check that the SAI implementation dependency is configured.  The Bazel build
# currently only supports building with a real SAI implementation.
# build-helper.py adds sai_impl to the fboss manifest when preparing a SAI build.
FBOSS_MANIFEST="$REPO_ROOT/build/fbcode_builder/manifests/fboss"
if [ -f "$FBOSS_MANIFEST" ] && ! grep -q '^sai_impl$' "$FBOSS_MANIFEST"; then
  echo "ERROR: Bazel build requires a SAI implementation (sai_impl not found in fboss manifest)." >&2
  echo "  Run build-helper.py first to configure the SAI implementation:" >&2
  echo "    ./fboss/oss/scripts/build-helper.py <libsai_impl_path> <experiments_path> <output_path> <version>" >&2
  exit 1
fi

# Regenerate Bazel BUILD files if any input (BUCK, .bzl, manifests) changed.
# Uses a content checksum stored in MODULE.bazel to skip when nothing changed.
BAZELIFY_ARGS="--if-needed -r $REPO_ROOT"
if [ -n "${GETDEPS_CACHE_URL:-}" ]; then
  BAZELIFY_ARGS="$BAZELIFY_ARGS --cache-url $GETDEPS_CACHE_URL"
fi
python3 "$SCRIPT_DIR/bazelify.py" $BAZELIFY_ARGS

# Run sccache setup — it writes .bazel.d/sccache.bazelrc.
"$SCRIPT_DIR/bazel-sccache-setup.sh"

# Generate remote-cache.bazelrc if BAZEL_REMOTE_CACHE_URL is configured.
mkdir -p "$BAZEL_D"
REMOTE_RC="$BAZEL_D/remote-cache.bazelrc"
if [ -n "${BAZEL_REMOTE_CACHE_URL:-}" ]; then
  cat >"$REMOTE_RC" <<EOF
# Auto-generated from fboss-build.env -- do not edit
build --remote_cache=$BAZEL_REMOTE_CACHE_URL
# Do not upload from dev machines: without sandboxing, actions can consume
# undeclared inputs that poison the shared cache. CI should override this
# with --remote_upload_local_results=true.
build --remote_upload_local_results=false
build --remote_local_fallback=true
EOF
else
  rm -f "$REMOTE_RC"
fi

# Assemble .bazel.d/*.bazelrc fragments into .bazelrc.d.
# Use a temp file + mv for atomicity (safe under concurrent bazel invocations).
if [ -d "$BAZEL_D" ]; then
  TMP="$(mktemp "$BAZELRC_D.XXXXXX")"
  echo "# Auto-assembled from .bazel.d/*.bazelrc by bazel.sh -- do not edit" >"$TMP"
  for f in "$BAZEL_D"/*.bazelrc; do
    [ -f "$f" ] || continue
    echo "" >>"$TMP"
    echo "# --- $(basename "$f") ---" >>"$TMP"
    cat "$f" >>"$TMP"
  done
  mv "$TMP" "$BAZELRC_D"
fi

# When running tests, append site-local exclusions as negative target patterns.
# Define BAZEL_TEST_EXCLUDE_TARGETS as a bash array in fboss-build.env, e.g.:
#   BAZEL_TEST_EXCLUDE_TARGETS=(
#     "//fboss/fsdb/client/test:fsdb_client_tests"
#   )
if [ "${1:-}" = "test" ] && [ "${#BAZEL_TEST_EXCLUDE_TARGETS[@]}" -gt 0 ]; then
  has_sep=false
  for arg in "$@"; do
    [ "$arg" = "--" ] && {
      has_sep=true
      break
    }
  done
  excludes=()
  $has_sep || excludes+=("--")
  for target in "${BAZEL_TEST_EXCLUDE_TARGETS[@]}"; do
    excludes+=("-$target")
  done
  exec /usr/local/bin/bazel "$@" "${excludes[@]}"
fi
exec /usr/local/bin/bazel "$@"
