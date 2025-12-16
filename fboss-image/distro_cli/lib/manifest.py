# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Image Manifest handling."""

import json
import logging
import sys
from pathlib import Path
from typing import Any, ClassVar

logger = logging.getLogger(__name__)


class ImageManifest:
    """Represents an FBOSS image manifest."""

    REQUIRED_FIELDS: ClassVar[list[str]] = ["distribution_formats", "kernel"]

    def __init__(self, manifest_path: Path):
        self.manifest_path = manifest_path.resolve()
        self.manifest_dir = self.manifest_path.parent
        self.data = self._load_manifest()
        self._validate_manifest()

    def _load_manifest(self) -> dict[str, Any]:
        """Load and validate the manifest file."""
        try:
            with self.manifest_path.open() as f:
                data = json.load(f)
            logger.info(f"Loaded manifest: {self.manifest_path}")
            return data
        except FileNotFoundError:
            logger.error(f"Manifest file not found: {self.manifest_path}")
            sys.exit(1)
        except json.JSONDecodeError as e:
            logger.error(f"Invalid JSON in manifest: {e}")
            sys.exit(1)

    def _validate_manifest(self):
        """Validate manifest has required fields."""
        missing_required = [
            field for field in self.REQUIRED_FIELDS if field not in self.data
        ]
        if missing_required:
            logger.error(
                f"Missing required fields in manifest: {', '.join(missing_required)}"
            )
            sys.exit(1)

    def has_component(self, component: str) -> bool:
        """Check if component is present in manifest."""
        return component in self.data

    def resolve_path(self, path: str) -> Path:
        """Resolve a path relative to the manifest file."""
        if path.startswith("http://") or path.startswith("https://"):
            return path
        return (self.manifest_dir / path).resolve()
