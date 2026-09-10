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
