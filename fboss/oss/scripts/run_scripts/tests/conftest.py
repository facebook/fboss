# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Shared pytest fixtures for TestRunner unit tests."""

import json
import os
import sys
import tempfile

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from argparse import ArgumentParser

from run_test import TestRunner


class StubTestRunner(TestRunner):
    """Concrete TestRunner with all abstract methods stubbed for testing."""

    def _get_config_path(self):
        return "/tmp/test.conf"

    def _get_known_bad_tests_file(self):
        return ""

    def _get_unsupported_tests_file(self):
        return ""

    def _get_test_binary_name(self):
        return "test_binary"

    def _get_sai_replayer_logging_flags(self, sai_replayer_log_path):
        return []

    def _get_sai_logging_flags(self):
        return []

    def _get_warmboot_check_file(self):
        return "/tmp/can_warm_boot"

    def _get_test_run_args(self, conf_file):
        return ["--config", conf_file]

    def _setup_coldboot_test(self, sai_replayer_log_path=None):
        pass

    def _setup_warmboot_test(self, sai_replayer_log_path=None):
        pass

    def _end_run(self):
        pass

    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        pass

    def _filter_tests(self, tests):
        return tests


@pytest.fixture
def runner():
    """StubTestRunner instance for testing concrete TestRunner methods."""
    return StubTestRunner()


@pytest.fixture
def known_bad_tests_json():
    """Temp JSON file with known bad test entries for two keys."""
    data = {
        "known_bad_tests": {
            "brcm/8.2/8.2/tomahawk": [
                {"test_name_regex": "HwVlanTest\\.VlanDelete", "reason": "T12345"},
                {"test_name_regex": "HwAcl.*", "reason": "T67890"},
            ],
            "brcm/8.2/8.2/tomahawk/mono": [
                {"test_name_regex": "HwRouteTest\\.MonoOnly", "reason": "T11111"},
            ],
        }
    }
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".json") as f:
        json.dump(data, f)
        path = f.name
    yield path
    os.unlink(path)


@pytest.fixture
def unsupported_tests_json():
    """Temp JSON file with unsupported test entries."""
    data = {
        "unsupported_tests": {
            "brcm/8.2/8.2/tomahawk": [
                {"test_name_regex": "HwMirrorTest\\..*", "reason": "Not supported"},
            ],
        }
    }
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".json") as f:
        json.dump(data, f)
        path = f.name
    yield path
    os.unlink(path)
