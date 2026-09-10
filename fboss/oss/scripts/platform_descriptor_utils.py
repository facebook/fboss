# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import annotations

import pathlib
import shutil


PLATFORM_DESCRIPTOR_ARCHIVE_ROOT = pathlib.PurePosixPath(
    "share/platform_descriptors/platforms"
)


def get_platform_descriptor_paths(
    platform_configs_dir: pathlib.Path,
) -> dict[pathlib.Path, str]:
    # Keep selection and relative paths aligned with
    # //fboss/configs:vendor_platform_descriptors in fboss/configs/BUCK.
    paths = {}
    descriptor_paths = sorted(
        platform_configs_dir.glob(
            "**/platform_mapping/generated/platform_descriptor.json"
        )
    )
    if not descriptor_paths:
        raise RuntimeError(
            f"No generated platform descriptors found under {platform_configs_dir}"
        )

    for descriptor_path in descriptor_paths:
        generated_dir = descriptor_path.parent
        mapping_path = generated_dir / "platform_mapping.json"
        if not mapping_path.is_file():
            raise RuntimeError(
                f"Platform descriptor {descriptor_path} has no sibling "
                f"{mapping_path.name}"
            )
        for source in sorted(generated_dir.glob("*.json")):
            relative_path = source.relative_to(platform_configs_dir)
            paths[source] = str(PLATFORM_DESCRIPTOR_ARCHIVE_ROOT / relative_path)
    return paths


def copy_platform_descriptors(
    platform_configs_dir: pathlib.Path, package_root: pathlib.Path
) -> None:
    for source, relative_destination in get_platform_descriptor_paths(
        platform_configs_dir
    ).items():
        destination = package_root / relative_destination
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
