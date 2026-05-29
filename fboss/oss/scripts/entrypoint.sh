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
    mkdir -p "$FBOSS_SRC/build"
    rm -rf "$FBOSS_SRC/build/fbcode_builder"
    ln -sfn "$FBCODE/fboss/github/CMakeLists.txt" "$FBOSS_SRC/CMakeLists.txt"
    ln -sfn "$FBCODE/fboss/github/functions.cmake" "$FBOSS_SRC/functions.cmake"
    ln -sfn "$FBCODE/fboss/github/cmake" "$FBOSS_SRC/cmake"
    ln -sfn "$FBCODE/fboss" "$FBOSS_SRC/fboss"
    ln -sfn "$FBCODE/fboss/common" "$FBOSS_SRC/common"
    ln -sfn "$FBCODE/configerator" "$FBOSS_SRC/configerator"
    cp -r "$FBCODE/opensource/fbcode_builder" "$FBOSS_SRC/build/fbcode_builder"
    ln -sfn "$FBCODE/fboss/github/fboss-image" "$FBOSS_SRC/fboss-image"
    ln -sfn "$FBCODE/fboss/github/sdk_dependencies.txt" "$FBOSS_SRC/sdk_dependencies.txt"
    ln -sfn "$FBCODE/fboss/github/README.md" "$FBOSS_SRC/README.md"
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
