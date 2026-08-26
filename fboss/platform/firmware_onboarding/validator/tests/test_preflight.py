# pyre-strict

"""Tests for the combined Meta-side pre-flight CLI."""

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path
from typing import Any

from fboss.platform.firmware_onboarding.validator.preflight import run_preflight


_VALID_SPEC: dict[str, Any] = {
    "spec_version": "1.0",
    "platform": {
        "name": "icecube800bc",
        "silicon": [{"name": "TH6", "pcie_address": "00:01.0"}],
    },
    "components": [
        {
            "name": "bios",
            "platform_identifier": "icecube800bc",
            "location": "Main Board",
            "display_location": "mb",
            "field_replaceable": False,
            "firmware": {"priority": 1, "shared_with_platforms": []},
        },
    ],
}


def _spec_with(**overrides: Any) -> dict[str, Any]:
    out = copy.deepcopy(_VALID_SPEC)
    out.update(overrides)
    return out


def _write_spec(configs_root: Path, platform: str, spec: dict[str, Any]) -> Path:
    platform_dir = configs_root / platform
    platform_dir.mkdir(parents=True, exist_ok=True)
    spec_path = platform_dir / "spec.json"
    spec_path.write_text(json.dumps(spec))
    return spec_path


class PreflightTest(unittest.TestCase):
    def test_baseline_valid_spec_passes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            spec_path = _write_spec(Path(tmp), "icecube800bc", _VALID_SPEC)
            self.assertEqual(run_preflight(spec_path), [])

    def test_structural_failure_surfaces_schema_error(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            spec = _spec_with()
            del spec["spec_version"]
            spec_path = _write_spec(Path(tmp), "icecube800bc", spec)
            errors = run_preflight(spec_path)
            self.assertTrue(any("[schema]" in e for e in errors))

    def test_semantic_failure_surfaces_semantic_error(self) -> None:
        # `before_first_underscore` with no underscore in name → semantic error.
        with tempfile.TemporaryDirectory() as tmp:
            spec = _spec_with()
            spec["components"][0]["instances"] = {
                "range": "1-2",
                "position": "before_first_underscore",
            }
            spec_path = _write_spec(Path(tmp), "icecube800bc", spec)
            errors = run_preflight(spec_path)
            self.assertTrue(any("[semantic]" in e for e in errors))
