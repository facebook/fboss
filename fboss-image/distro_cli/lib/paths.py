# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Path resolution utilities for FBOSS image builder."""

import os
from functools import lru_cache
from pathlib import Path


@lru_cache(maxsize=1)
def get_root_dir() -> Path:
    """Find the repository root directory.

    If FBOSS_ROOT is set, uses that directly - this is particularly
    helpful when using symlink-mapped directories. Otherwise walks up from
    the current file until finding 'fboss-image' and 'fboss' directories.

    The result is cached after the first call for performance.

    Returns:
        Path to repository root directory

    Raises:
        RuntimeError: If the root directory cannot be determined
    """
    override = os.environ.get("FBOSS_ROOT")
    if override:
        root = Path(override)
        if root.is_dir():
            return root
        raise RuntimeError(f"FBOSS_ROOT={override} is not a valid directory")

    current = Path(__file__).resolve()

    for parent in current.parents:
        if (parent / "fboss-image").is_dir() and (parent / "fboss").is_dir():
            return parent

    raise RuntimeError(
        f"Could not find repository root from {current}. "
        f"Expected to find 'fboss-image' and 'fboss' directories in parent path."
    )


def get_abs_path(path_parts: str | list[str]) -> Path:
    """Get absolute path by joining the parent of 'fboss-image' with provided path parts.

    Args:
        path_parts: Either a string like "fboss-image/distro_cli/tools" or
                   a list like ["fboss-image", "distro_cli", "tools"]

    Returns:
        Absolute path

    Examples:
        get_abs_path("fboss-image/distro_cli/tools")
        get_abs_path(["fboss-image", "distro_cli", "tools"])
        get_abs_path("fboss/oss/scripts")
    """
    root = get_root_dir()

    if isinstance(path_parts, str):
        return root / path_parts
    return root.joinpath(*path_parts)
