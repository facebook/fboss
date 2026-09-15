# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for fsdb_service_utils FSDB systemd service lifecycle."""

import os
import sys
from unittest.mock import patch

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import fboss_test_runner.services.fsdb_service_utils as fsdb_service_utils
from fboss_test_runner.services.fsdb_service_utils import (
    cleanup_fsdb_service,
    setup_and_start_fsdb_service,
)


class TestCleanupFsdbService:
    def test_stops_all_three_variants_and_pkills(self):
        with patch(
            "fboss_test_runner.services.service_utils.subprocess.run"
        ) as mock_run:
            cleanup_fsdb_service()
        commands = "\n".join(c.args[0] for c in mock_run.call_args_list)
        assert "systemctl stop fsdb.service" in commands
        assert "systemctl stop fsdb_service_for_testing" in commands
        assert "systemctl stop fsdb_service_oss" in commands
        assert "systemctl disable fsdb_service_oss" in commands
        assert "systemctl daemon-reload" in commands
        assert "pkill -f fsdb_service_oss" in commands


class TestSetupPreconditions:
    def _patches(self, monkeypatch, tmp_path):
        monkeypatch.setattr(
            fsdb_service_utils,
            "_FSDB_SERVICE_UNIT_FILE_PATH",
            str(tmp_path / "fsdb_oss.service"),
        )
        monkeypatch.setattr(
            fsdb_service_utils.service_utils,
            "write_rsyslog_conf",
            lambda *a, **k: None,
        )

    def test_raises_when_binary_missing(self, monkeypatch, tmp_path):
        self._patches(monkeypatch, tmp_path)
        monkeypatch.setattr(
            fsdb_service_utils,
            "_DEFAULT_OSS_FSDB_SERVICE_BINARY",
            "missing_fsdb_bin",
        )
        monkeypatch.setattr(
            fsdb_service_utils.service_utils.shutil, "which", lambda _: None
        )
        with pytest.raises(Exception, match="fsdb_service binary"):
            setup_and_start_fsdb_service()

    def test_raises_when_config_missing(self, monkeypatch, tmp_path):
        self._patches(monkeypatch, tmp_path)
        monkeypatch.setattr(
            fsdb_service_utils, "_DEFAULT_OSS_FSDB_SERVICE_BINARY", sys.executable
        )
        with pytest.raises(Exception, match="fsdb_service config path"):
            setup_and_start_fsdb_service(
                fsdb_service_config_path=str(tmp_path / "missing.conf")
            )

    def test_resolves_config_path_before_building_unit(self, monkeypatch, tmp_path):
        monkeypatch.chdir(tmp_path)
        relative_config_path = "share/link_test_configs/fsdb.conf"

        with patch(
            "fboss_test_runner.services.fsdb_service_utils.service_utils"
        ) as mock_svc:
            setup_and_start_fsdb_service(fsdb_service_config_path=relative_config_path)

        expected_config_path = tmp_path / relative_config_path
        mock_svc.validate_path.assert_any_call(
            str(expected_config_path), "fsdb_service config path"
        )
        assert (
            f"--fsdb_config={expected_config_path}"
            in mock_svc.build_unit_file_content.call_args.kwargs["exec_start_cmd"]
        )

    def test_default_config_uses_sourced_package_data(self, monkeypatch, tmp_path):
        package_data = tmp_path / "share"
        expected_config_path = package_data / "link_test_configs/fsdb.conf"
        monkeypatch.setenv("FBOSS_DATA", str(package_data))

        with patch(
            "fboss_test_runner.services.fsdb_service_utils.service_utils"
        ) as mock_svc:
            setup_and_start_fsdb_service()

        mock_svc.validate_path.assert_any_call(
            str(expected_config_path), "fsdb_service config path"
        )
        assert (
            f"--fsdb_config={expected_config_path}"
            in mock_svc.build_unit_file_content.call_args.kwargs["exec_start_cmd"]
        )

    def test_empty_package_data_uses_packaged_fallback(self, monkeypatch):
        expected_config_path = "/opt/fboss/share/link_test_configs/fsdb.conf"
        monkeypatch.setenv("FBOSS_DATA", "")

        with patch(
            "fboss_test_runner.services.fsdb_service_utils.service_utils"
        ) as mock_svc:
            setup_and_start_fsdb_service()

        mock_svc.validate_path.assert_any_call(
            expected_config_path, "fsdb_service config path"
        )


class TestStartColdVsWarmBoot:
    def test_cold_boot_passes_warm_boot_false(self):
        with patch(
            "fboss_test_runner.services.fsdb_service_utils.service_utils"
        ) as mock_svc:
            mock_svc.validate_path = lambda *a: None
            setup_and_start_fsdb_service(is_warm_boot=False)
        mock_svc.start_service.assert_called_once()
        assert mock_svc.start_service.call_args[1]["is_warm_boot"] is False

    def test_warm_boot_passes_warm_boot_true(self):
        with patch(
            "fboss_test_runner.services.fsdb_service_utils.service_utils"
        ) as mock_svc:
            mock_svc.validate_path = lambda *a: None
            setup_and_start_fsdb_service(is_warm_boot=True)
        mock_svc.start_service.assert_called_once()
        assert mock_svc.start_service.call_args[1]["is_warm_boot"] is True
