# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for BcmTestRunner."""

import pytest
from run_test import BcmTestRunner


@pytest.fixture
def bcm_runner():
    return BcmTestRunner()


class TestConstants:
    def test_default_paths_and_binary(self, bcm_runner):
        assert bcm_runner._get_config_path() == "/etc/coop/bcm.conf"
        assert bcm_runner._get_test_binary_name() == "/opt/fboss/bin/bcm_test"
        assert bcm_runner._get_known_bad_tests_file() == ""
        assert bcm_runner._get_unsupported_tests_file() == ""
        assert bcm_runner._get_sai_replayer_logging_flags("/any/path") == []
        assert bcm_runner._get_sai_replayer_logging_flags(None) == []
        assert bcm_runner._get_test_run_args("dummy.conf") == []


class TestSaiLoggingFlagsSignature:
    def test_no_arg_signature_returns_empty(self, bcm_runner):
        assert bcm_runner._get_sai_logging_flags() == []

    def test_calling_with_sai_logging_arg_raises_typeerror(self, bcm_runner):
        with pytest.raises(TypeError):
            bcm_runner._get_sai_logging_flags("WARN")
