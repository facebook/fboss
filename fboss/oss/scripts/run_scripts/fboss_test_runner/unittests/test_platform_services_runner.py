# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for PlatformServicesTestRunner.

The runner has two unique behaviors worth testing: (1) per-test-type binary
selection via a dict map with a fallback default, and (2) the multi-type
iteration in run_test that iterates TEST_TYPE_CHOICES when --type is omitted."""

from unittest.mock import MagicMock, patch

import pytest
from fboss_test_runner.runners.platform_services_test_runner import (
    PlatformServicesTestRunner,
)


@pytest.fixture
def platform_runner():
    return PlatformServicesTestRunner()


def _make_args(**overrides):
    args = MagicMock()
    args.type = None
    args.config = None
    args.list_tests = False
    args.skip_known_bad_tests = None
    args.sai_logging = "WARN"
    args.fboss_logging = "WARN"
    args.test_run_timeout = 300
    args.run_on_reference_board = False
    for k, v in overrides.items():
        setattr(args, k, v)
    return args


class TestBinaryNameByType:
    """The binary name comes from a dict map keyed on args.type. Unknown
    types fall back to platform_hw_test."""

    @pytest.mark.parametrize(
        "test_type,expected",
        [
            ("platform_hw_test", "platform_hw_test"),
            ("data_corral_service_hw_test", "data_corral_service_hw_test"),
            ("fan_service_hw_test", "fan_service_hw_test"),
            ("fw_util_hw_test", "fw_util_hw_test"),
            ("sensor_service_hw_test", "sensor_service_hw_test"),
            ("weutil_hw_test", "weutil_hw_test"),
            ("platform_manager_hw_test", "platform_manager_hw_test"),
        ],
    )
    def test_known_types_map_to_their_binary(
        self, platform_runner, test_type, expected
    ):
        with patch.object(platform_runner, "args", new=_make_args(type=test_type)):
            assert platform_runner._get_test_binary_name() == expected

    def test_unknown_type_falls_back_to_platform_hw_test(self, platform_runner):
        with patch.object(
            platform_runner, "args", new=_make_args(type="not_a_real_type")
        ):
            assert platform_runner._get_test_binary_name() == "platform_hw_test"


class TestRunTestMultiTypeIteration:
    """When --type is omitted, run_test iterates over all 7 TEST_TYPE_CHOICES,
    setting args.type for each pass. This is the unique runner-level behavior
    (vs the base TestRunner's single-type loop)."""

    def test_iterates_all_test_types_when_type_none(self, platform_runner):
        seen_types = []
        # Spy on _get_tests_to_run to capture the type set at each iteration.
        runner_args = _make_args(list_tests=False)

        def capture_type():
            seen_types.append(runner_args.type)
            return ["FakeTest.A"]

        with (
            patch.object(platform_runner, "args", new=runner_args),
            patch("shutil.which", return_value="/opt/fboss/bin/platform_hw_test"),
            patch.object(platform_runner, "_initialize_test_lists"),
            patch.object(
                platform_runner, "_get_tests_to_run", side_effect=capture_type
            ),
            patch.object(platform_runner, "_filter_tests", side_effect=lambda t: t),
            patch.object(
                platform_runner, "_backup_and_modify_config", side_effect=lambda c: c
            ),
            patch.object(platform_runner, "_run_tests", return_value=[]),
            patch.object(platform_runner, "_print_output_summary"),
        ):
            platform_runner.run_test(runner_args)

        # All 7 platform test types are iterated, in the order of TEST_TYPE_CHOICES.
        assert seen_types == list(PlatformServicesTestRunner.TEST_TYPE_CHOICES)
