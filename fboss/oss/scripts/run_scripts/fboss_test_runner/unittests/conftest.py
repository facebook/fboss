# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Shared pytest fixtures for TestRunner unit tests."""

import json
import os
import sys
import tempfile
from unittest.mock import Mock

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import run_test
from fboss_test_runner.runners.test_runner import TestRunner

# `run_test.args` is populated at runtime by argparse inside main(), so the
# attribute does not exist at import time. Declare it here so unit tests can
# patch it without `create=True` — which means a rename of `args` in run_test.py
# will surface as a loud AttributeError at patch time instead of being silently
# fabricated by mock.
run_test.args = None


class StubTestRunner(TestRunner):
    """Concrete TestRunner with abstract methods stubbed for testing."""

    def _get_test_binary_name(self):
        return "test_binary"

    def _get_warmboot_check_file(self):
        return "/tmp/can_warm_boot"

    def _get_test_run_args(self, conf_file):
        return ["--config", conf_file]


@pytest.fixture
def runner():
    """StubTestRunner instance for testing concrete TestRunner methods."""
    return StubTestRunner()


@pytest.fixture
def mock_args():
    """Create a mock args object with common attributes used across runners."""
    args = Mock()
    args.filter_file = None
    args.list_tests = False
    args.config = None
    args.mgmt_if = "eth0"
    args.platform_mapping_override_path = None
    args.fruid_path = None
    args.sai_logging = "WARN"
    args.fboss_logging = "WARN"
    args.test_run_timeout = 300
    args.skip_known_bad_tests = None
    args.extra_gflags = None
    return args


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
