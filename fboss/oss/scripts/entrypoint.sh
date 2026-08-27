#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#
# Creates fboss_src/ staging layout when running on a Meta devserver.
# Expects fbcode/ mounted at /var/FBOSS/fbcode via docker run -v.
# No-op on GitHub CI where fboss_src comes from the repo clone.

FBCODE="/var/FBOSS/fbcode"
FBOSS_SRC="/var/FBOSS/fboss"

if [ "$FBOSS_META_ENV" = "1" ]; then
    export FBOSS_ROOT="$FBOSS_SRC"
    echo "export FBOSS_ROOT=$FBOSS_SRC" > /etc/profile.d/fboss.sh
fi

if [ "$FBOSS_META_ENV" = "1" ]; then
    bash "$FBCODE/fboss/facebook/oss_ci/setup_oss_src_dir.sh" "$FBCODE" "$FBOSS_SRC"

    # build/ is copied (not symlinked to the on-diff root build/) so we never
    # write into the mounted checkout at $FBCODE. Copy only when absent: applying
    # stable commits overwrites build/fbcode_builder with a pinned snapshot, so a
    # re-run of this entrypoint must not clobber it back to the live checkout.
    mkdir -p "$FBOSS_SRC/build"
    [ -d "$FBOSS_SRC/build/fbcode_builder" ] || cp -r "$FBCODE/opensource/fbcode_builder" "$FBOSS_SRC/build/fbcode_builder"

    # fwdproxy (baked into this image as http_proxy) can refuse CONNECT to
    # ftpmirror.gnu.org, which getdeps uses for the GNU manifests. Probe it with
    # a one-byte range request and only fall back to mirrors.kernel.org — the
    # same /gnu tree — when it is actually unreachable, so we track upstream
    # whenever the proxy allows it. Rewrites the copy, never the manifests in
    # the mounted checkout, and runs on every entrypoint invocation so a pinned
    # fbcode_builder snapshot gets it too. getdeps still verifies each archive's
    # sha256, so the mirror swap cannot change what gets built.
    manifests_dir="$FBOSS_SRC/build/fbcode_builder/manifests"
    probe_url=$(grep -rhom1 'https://ftpmirror\.gnu\.org/[^"[:space:]]*' \
        "$manifests_dir" 2>/dev/null | head -1)
    if [ -n "$probe_url" ]; then
        if curl -fsSL --max-time 20 --retry 0 -r 0-0 -o /dev/null "$probe_url"; then
            echo "[fboss] ftpmirror.gnu.org reachable; leaving GNU manifests as-is"
        else
            echo "[fboss] WARNING: ftpmirror.gnu.org is unreachable from this" \
                 "container (proxy: ${https_proxy:-none}). Rewriting GNU manifest" \
                 "URLs in $manifests_dir to mirrors.kernel.org. Archive sha256s" \
                 "are still verified." >&2
            grep -rlZ 'ftpmirror\.gnu\.org' "$manifests_dir" |
                xargs -0 --no-run-if-empty \
                    sed -i 's|ftpmirror\.gnu\.org|mirrors.kernel.org|g'
        fi
    fi

    # devserver-only extras (docker/lint/docs), not needed by the on-diff build.
    cp -r "$FBCODE/fboss/github/.pre-commit-config.yaml" "$FBOSS_SRC/.pre-commit-config.yaml"
    ln -sfn "$FBCODE/fboss/github/requirements-dev.txt" "$FBOSS_SRC/requirements-dev.txt"
    ln -sfn "$FBCODE/fboss/github/getdeps.sh" "$FBOSS_SRC/getdeps.sh"
    ln -sfn "$FBCODE/fboss/github/docs" "$FBOSS_SRC/docs"
    ln -sfn "$FBCODE/fboss/github/ruff.toml" "$FBOSS_SRC/ruff.toml"
    ln -sfn "$FBCODE/fboss/github/LICENSE" "$FBOSS_SRC/LICENSE"
    ln -sfn "$FBCODE/fboss/github/PATENTS" "$FBOSS_SRC/PATENTS"
    ln -sfn "$FBCODE/fboss/github/CONTRIBUTING.md" "$FBOSS_SRC/CONTRIBUTING.md"
    ln -sfn "$FBCODE/fboss/github/CODE_OF_CONDUCT.md" "$FBOSS_SRC/CODE_OF_CONDUCT.md"
    ln -sfn "$FBCODE/fboss/github/BUILD.md" "$FBOSS_SRC/BUILD.md"
    ln -sfn "$FBCODE/fboss/github/.clang-format" "$FBOSS_SRC/.clang-format"
    ln -sfn "$FBCODE/fboss/github/.gitignore" "$FBOSS_SRC/.gitignore"
fi

exec "$@"
