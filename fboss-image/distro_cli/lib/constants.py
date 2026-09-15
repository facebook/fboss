# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Constants for FBOSS image builder."""

# Docker image names
FBOSS_BUILDER_IMAGE = "fboss_builder"

# Every component a manifest may declare. A component's name is also the
# directory name it is staged into under /repos, which config.sh dispatches on,
# so these strings are a contract with the in-image build and cannot be renamed
# on one side alone.
IMAGE_COMPONENTS = (
    "kernel",
    "other_dependencies",
    "fboss-platform-stack",
    "bsps",
    "npu_sai",
    "phy_sai",
    "fboss-forwarding-stack",
    "image_build_hooks",
)

# Manifest keys that are not components.
MANIFEST_METADATA_FIELDS = ("distribution_formats",)

# Environment variable naming where pre-built artifacts are fetched from.
# Manifests reference ${ARTIFACT_BASE}/<relative/path>; the relative path is
# the same everywhere, only the base differs between a Meta build, an external
# vendor's mirror, and a local directory.
ARTIFACT_BASE_VAR = "ARTIFACT_BASE"

# Shown in the error when ARTIFACT_BASE is required but unset.
DEFAULT_ARTIFACT_BUCKET = "fboss.oss.platform.artifacts/tree"
