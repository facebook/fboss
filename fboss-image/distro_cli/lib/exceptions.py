# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Exception definitions for FBOSS image builder."""


class FbossImageError(Exception):
    """Base exception for all fboss-image errors.

    All custom exceptions in the fboss-image tool should inherit from this.
    This allows the top-level CLI handler to catch and format errors appropriately
    before returning a non-zero exit code.
    """


class BuildError(FbossImageError):
    """Build command failed.

    Raised when a component build fails, including:
    - Build script/command not found
    - Build process returned non-zero exit code
    - Build output validation failed
    """


class ManifestError(FbossImageError):
    """Manifest parsing or validation failed.

    Raised when:
    - Manifest file not found or invalid JSON
    - Required fields missing
    - Invalid manifest structure
    """


class ArtifactError(FbossImageError):
    """Artifact operation failed.

    Raised when:
    - Expected artifact not found after build
    - Artifact download failed
    - Artifact cache operation failed
    """


class ComponentError(FbossImageError):
    """Component-specific error.

    Raised when:
    - Component not found in manifest
    - Component configuration invalid
    - Component builder not implemented
    """
