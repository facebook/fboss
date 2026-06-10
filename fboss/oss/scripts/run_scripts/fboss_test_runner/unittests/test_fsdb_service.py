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
            "_DEFAULT_OSS_FSDB_SERVICE_PATH",
            str(tmp_path / "missing_fsdb_bin"),
        )
        with pytest.raises(Exception, match="fsdb_service binary"):
            setup_and_start_fsdb_service()

    def test_raises_when_config_missing(self, monkeypatch, tmp_path):
        self._patches(monkeypatch, tmp_path)
        monkeypatch.setattr(
            fsdb_service_utils, "_DEFAULT_OSS_FSDB_SERVICE_PATH", sys.executable
        )
        with pytest.raises(Exception, match="fsdb_service config path"):
            setup_and_start_fsdb_service(
                fsdb_service_config_path=str(tmp_path / "missing.conf")
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
