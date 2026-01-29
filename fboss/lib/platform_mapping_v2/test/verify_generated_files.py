#!/usr/bin/env python3
# pyre-strict

import filecmp
import os
import sys
import unittest
from typing import Dict, List

from fboss.lib.platform_mapping_v2.gen import (
    generate_platform_mappings,
    INPUT_DIR as input_dir,
)


class TestVerifyPlatformMappingGeneratedFiles(unittest.TestCase):
    """
    GitHub Actions test that verifies a PR includes all generated files caused by code changes. This ensures that
    if a vendor changes any platform mapping generation code, we can guarantee it doesn't affect any platform mapping
    other than those we expect before merging the changes.

    This test will be run in OSS for all open-sourced platforms.
    """

    _OSS_MULTI_NPU_SUPPORTED_PLATFORMS: Dict[bool, List[str]] = {
        False: [
            "montblanc",
            "montblanc_odd_ports_8x100G",
            "minipack3n",
            "minipack3bta",
            "meru800bia",
            "meru800bia_dual_stage_rdsw",
            "meru800bia_dual_stage_edsw",
            "meru800bia_100g_nif_port_breakout",
            "meru800bia_800g",
            "meru800bia_800g_hyperport",
            "meru800bia_single_stage_192_rdsw_40_fdsw_32_edsw",
            "meru800bia_single_stage_192_rdsw_40_fdsw_32_edsw_800g",
            "meru800bia_800g_uniform_local_offset",
            "meru800bia_uniform_local_offset",
            "janga800bic_dctype1_prod",
            "janga800bic_dctype1_test_fixture",
            "tahan800bc_test_fixture",
            "tahan800bc_chassis",
            "icecube800bc",
            "icetea800bc",
            "tahansb800bc",
            "wedge800bact",
            "wedge800cact",
            "icecube800banw",
        ],
        True: [
            "meru800bfa",
            "meru400bfu",
            "janga800bic_dctypef_prod",
            "janga800bic_dctypef_test_fixture",
            "ladakh800bcls",
        ],
    }
    _FBCODE_GENERATED_DIR: str = (
        "fboss/lib/platform_mapping_v2/generated_platform_mappings"
    )
    _TMP_GENERATED_DIR: str = "/tmp/generated_platform_mappings/"

    def _clear_tmp_generated_mappings(self) -> None:
        """
        Clears out any files in the /tmp/generated_platform_mappings directory.
        """
        if os.path.exists(self._TMP_GENERATED_DIR):
            for filename in os.listdir(self._TMP_GENERATED_DIR):
                file_path = os.path.join(self._TMP_GENERATED_DIR, filename)
                try:
                    if os.path.isfile(file_path) or os.path.islink(file_path):
                        os.unlink(file_path)
                    elif os.path.isdir(file_path):
                        os.rmdir(file_path)
                except Exception as e:
                    print(f"Failed to delete {file_path}. Reason: {e}", file=sys.stderr)

    def _generate_all_oss_platform_mappings_in_tmp(self) -> None:
        for is_multi_npu, platforms in self._OSS_MULTI_NPU_SUPPORTED_PLATFORMS.items():
            for platform in platforms:
                generate_platform_mappings(
                    input_dir + platform,
                    self._TMP_GENERATED_DIR,
                    platform,
                    is_multi_npu,
                )

    def test_generated_files_match(self) -> None:
        """
        This test:
        1. Runs platform mapping generation on all open-sourced platforms, specifying output directory to /tmp/generated_platform_mappings.
        2. Compares /tmp generated files and /fbcode generated files (including changes from current PR).
        3. Verifies all generated files match, raises AssertionError if not.
        """
        self._clear_tmp_generated_mappings()
        print(
            f"Cleared any existing files in {self._TMP_GENERATED_DIR}", file=sys.stderr
        )

        self._generate_all_oss_platform_mappings_in_tmp()
        print(
            f"Generated all platform mappings in {self._TMP_GENERATED_DIR}",
            file=sys.stderr,
        )

        self.assertTrue(
            os.path.exists(self._FBCODE_GENERATED_DIR),
            f"Fbcode generated directory {self._FBCODE_GENERATED_DIR} not found",
        )
        self.assertTrue(
            os.path.exists(self._TMP_GENERATED_DIR),
            f"Tmp generated directory {self._TMP_GENERATED_DIR} not found",
        )
        print(
            f"Folder paths exist (fbcode: {self._FBCODE_GENERATED_DIR}, tmp: {self._TMP_GENERATED_DIR})",
            file=sys.stderr,
        )

        ref_files = os.listdir(self._FBCODE_GENERATED_DIR)
        gen_files = os.listdir(self._TMP_GENERATED_DIR)

        self.assertEqual(
            sorted(ref_files),
            sorted(gen_files),
            "Fbcode and tmp generated files don't match",
        )

        for filename in ref_files:
            print(f"Verifying file {filename}", file=sys.stderr)
            ref_path = os.path.join(self._FBCODE_GENERATED_DIR, filename)
            gen_path = os.path.join(self._TMP_GENERATED_DIR, filename)
            self.assertTrue(
                filecmp.cmp(ref_path, gen_path, shallow=False),
                f"File contents don't match for {filename}",
            )


def run_tests() -> None:
    # Provided for add_fb_python_executable callable
    suite = unittest.TestLoader().loadTestsFromTestCase(
        TestVerifyPlatformMappingGeneratedFiles
    )
    result = unittest.TextTestRunner().run(suite)

    if not result.wasSuccessful():
        raise Exception("Test failures.")
