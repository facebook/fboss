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

# pyre-strict

import pathlib
import tarfile
import tempfile
import unittest
from unittest import mock

from fboss.oss.scripts import package


class PackageTest(unittest.TestCase):
    @mock.patch.object(package, "_find_getdeps_libs", return_value={})
    def test_forwarding_package_includes_platform_descriptors(
        self,
        _mock_find_getdeps_libs: mock.MagicMock,
    ) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            platforms_dir = pathlib.Path(temp_dir) / "platforms"
            descriptor_dir = (
                platforms_dir
                / "accton"
                / "wedge800bact"
                / "platform_mapping"
                / "generated"
            )
            descriptor_dir.mkdir(parents=True)
            descriptor_path = descriptor_dir / "platform_descriptor.json"
            descriptor_path.write_text("{}")
            (descriptor_dir / "platform_mapping.json").write_text("{}")

            with mock.patch.object(package, "PLATFORM_CONFIGS_DIR", platforms_dir):
                production_files, _ = package._build_target(
                    "forwarding-stack", pathlib.Path("/build")
                )

            self.assertEqual(
                "share/platform_descriptors/platforms/accton/wedge800bact/"
                "platform_mapping/generated/platform_descriptor.json",
                production_files[descriptor_path],
            )

            tar_path = pathlib.Path(temp_dir) / "forwarding-stack.tar"
            package.write_tar(
                str(tar_path),
                {descriptor_path: production_files[descriptor_path]},
            )
            with tarfile.open(tar_path) as archive:
                self.assertIn(
                    "share/platform_descriptors/platforms/accton/wedge800bact/"
                    "platform_mapping/generated/platform_descriptor.json",
                    archive.getnames(),
                )

    @mock.patch.object(package, "get_platform_descriptor_paths", return_value={})
    @mock.patch.object(package, "_find_getdeps_libs", return_value={})
    def test_forwarding_packages_include_read_only_npu_sdk_metadata(
        self,
        _mock_find_getdeps_libs: mock.MagicMock,
        _mock_get_platform_descriptor_paths: mock.MagicMock,
    ) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            build_dir = pathlib.Path(temp_dir)
            metadata_path = (
                build_dir / "build" / "fboss" / package.NPU_SDK_METADATA_FILENAME
            )
            metadata_path.parent.mkdir(parents=True)
            metadata_path.write_text("{}\n")
            metadata_path.chmod(0o444)

            production_files, test_files = package._build_target(
                "forwarding-stack", build_dir
            )

            self.assertEqual(
                package.NPU_SDK_METADATA_ARCHIVE_PATH,
                production_files[metadata_path],
            )
            self.assertEqual(
                package.NPU_SDK_METADATA_ARCHIVE_PATH,
                test_files[metadata_path],
            )

            tar_path = build_dir / "metadata.tar"
            package.write_tar(
                str(tar_path),
                {metadata_path: package.NPU_SDK_METADATA_ARCHIVE_PATH},
            )
            with tarfile.open(tar_path) as archive:
                self.assertEqual(
                    0o444,
                    archive.getmember(package.NPU_SDK_METADATA_ARCHIVE_PATH).mode,
                )

    @mock.patch.object(package, "get_platform_descriptor_paths", return_value={})
    @mock.patch.object(package, "_find_getdeps_libs", return_value={})
    def test_forwarding_package_rejects_writable_npu_sdk_metadata(
        self,
        _mock_find_getdeps_libs: mock.MagicMock,
        _mock_get_platform_descriptor_paths: mock.MagicMock,
    ) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            build_dir = pathlib.Path(temp_dir)
            metadata_path = (
                build_dir / "build" / "fboss" / package.NPU_SDK_METADATA_FILENAME
            )
            metadata_path.parent.mkdir(parents=True)
            metadata_path.write_text("{}\n")

            with self.assertRaisesRegex(RuntimeError, "must be read-only"):
                package._build_target("forwarding-stack", build_dir)
