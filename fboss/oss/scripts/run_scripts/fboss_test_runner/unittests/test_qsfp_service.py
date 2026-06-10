# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for qsfp_service_utils QSFP systemd service lifecycle."""

import os
import sys
from unittest.mock import patch

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import fboss_test_runner.services.qsfp_service_utils as qsfp_service_utils
from fboss_test_runner.services.qsfp_service_utils import (
    _setup_qsfp_service,
    cleanup_qsfp_service,
    setup_and_start_qsfp_service,
)


def _redirect_paths(monkeypatch, tmp_path):
    monkeypatch.setattr(
        qsfp_service_utils,
        "_QSFP_SERVICE_UNIT_FILE_PATH",
        str(tmp_path / "qsfp_oss.service"),
    )
    monkeypatch.setattr(
        qsfp_service_utils.service_utils,
        "write_rsyslog_conf",
        lambda *a, **k: None,
    )


class TestCleanupQsfpService:
    def test_stops_all_three_variants_and_pkills(self):
        with patch(
            "fboss_test_runner.services.service_utils.subprocess.run"
        ) as mock_run:
            cleanup_qsfp_service()
        commands = "\n".join(c.args[0] for c in mock_run.call_args_list)
        assert "systemctl stop qsfp_service" in commands
        assert "systemctl stop qsfp_service_for_testing" in commands
        assert "systemctl stop qsfp_service_oss" in commands
        assert "systemctl disable qsfp_service_oss" in commands
        assert "pkill -f qsfp_service_oss" in commands


class TestSetupPreconditions:
    def test_raises_when_binary_missing(self, monkeypatch, tmp_path):
        _redirect_paths(monkeypatch, tmp_path)
        monkeypatch.setattr(
            qsfp_service_utils,
            "_DEFAULT_OSS_QSFP_SERVICE_PATH",
            str(tmp_path / "missing_qsfp_bin"),
        )
        config = tmp_path / "qsfp.conf"
        config.write_text("")
        with pytest.raises(Exception, match="qsfp_service binary"):
            setup_and_start_qsfp_service(qsfp_service_config_path=str(config))

    def test_raises_when_config_none(self, monkeypatch, tmp_path):
        _redirect_paths(monkeypatch, tmp_path)
        monkeypatch.setattr(
            qsfp_service_utils, "_DEFAULT_OSS_QSFP_SERVICE_PATH", sys.executable
        )
        with pytest.raises(Exception, match=r"qsfp_service config path.*is None"):
            setup_and_start_qsfp_service(qsfp_service_config_path=None)


class TestUnitFileExtraArgs:
    def _setup(self, monkeypatch, tmp_path):
        _redirect_paths(monkeypatch, tmp_path)
        monkeypatch.setattr(
            qsfp_service_utils, "_DEFAULT_OSS_QSFP_SERVICE_PATH", sys.executable
        )

    def test_fsdb_enabled_adds_ssl_preferred_flag(self, monkeypatch, tmp_path):
        self._setup(monkeypatch, tmp_path)
        config = tmp_path / "qsfp.conf"
        config.write_text("")
        with patch("fboss_test_runner.services.service_utils.subprocess.run"):
            _setup_qsfp_service(
                qsfp_service_config_path=str(config), is_fsdb_disabled=False
            )
        unit = (tmp_path / "qsfp_oss.service").read_text()
        assert "--fsdb_client_ssl_preferred=false" in unit
        assert "--publish_stats_to_fsdb=true" in unit

    def test_fsdb_disabled_omits_ssl_preferred_flag(self, monkeypatch, tmp_path):
        self._setup(monkeypatch, tmp_path)
        config = tmp_path / "qsfp.conf"
        config.write_text("")
        with patch("fboss_test_runner.services.service_utils.subprocess.run"):
            _setup_qsfp_service(
                qsfp_service_config_path=str(config), is_fsdb_disabled=True
            )
        unit = (tmp_path / "qsfp_oss.service").read_text()
        assert "--fsdb_client_ssl_preferred=false" not in unit

    def test_platform_mapping_override_appended_when_set(self, monkeypatch, tmp_path):
        self._setup(monkeypatch, tmp_path)
        config = tmp_path / "qsfp.conf"
        config.write_text("")
        pm = tmp_path / "pm.json"
        pm.write_text("{}")
        with patch("fboss_test_runner.services.service_utils.subprocess.run"):
            _setup_qsfp_service(
                qsfp_service_config_path=str(config),
                platform_mapping_override_path=str(pm),
            )
        unit = (tmp_path / "qsfp_oss.service").read_text()
        assert f"--platform_mapping_override_path {pm}" in unit


class TestStartColdVsWarmBoot:
    def test_cold_boot_passes_warm_boot_false(self):
        with patch(
            "fboss_test_runner.services.qsfp_service_utils.service_utils"
        ) as mock_svc:
            mock_svc.validate_path = lambda *a: None
            setup_and_start_qsfp_service(
                qsfp_service_config_path="/tmp/qsfp.conf",
                is_warm_boot=False,
            )
        mock_svc.start_service.assert_called_once()
        assert mock_svc.start_service.call_args[1]["is_warm_boot"] is False

    def test_warm_boot_passes_warm_boot_true(self):
        with patch(
            "fboss_test_runner.services.qsfp_service_utils.service_utils"
        ) as mock_svc:
            mock_svc.validate_path = lambda *a: None
            setup_and_start_qsfp_service(
                qsfp_service_config_path="/tmp/qsfp.conf",
                is_warm_boot=True,
            )
        mock_svc.start_service.assert_called_once()
        assert mock_svc.start_service.call_args[1]["is_warm_boot"] is True
