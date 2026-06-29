# pyre-strict

"""Tests for the FBOSS Platform Firmware Onboarding spec validator.

Each test mutates a known-good `_VALID_SPEC` dict to exercise one validation
rule. Failure tests assert that (a) the validator returns at least one error
and (b) the error message names the offending field so the vendor can fix it.
"""

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path
from typing import Any

from fboss.platform.firmware_onboarding.validator.spec_validator import (
    _Component,
    validate_spec_dict,
    validate_spec_file,
)


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
            "firmware": {
                "priority": 10,
                "shared_with_platforms": [],
            },
        },
        {
            "name": "smb_fpga",
            "platform_identifier": "icecube800bc",
            "location": "Switch Main Board",
            "display_location": "smb",
            "field_replaceable": False,
            "pcie_address": "01:00.0",
            "firmware": {
                "priority": 30,
                "shared_with_platforms": [],
            },
        },
        {
            "name": "pic_cpld",
            "platform_identifier": "icecube800bc",
            "location": "Physical Interface Card",
            "display_location": "pic",
            "field_replaceable": True,
            "instances": {"range": "1-2", "position": "at_end"},
            "firmware": {
                "priority": 40,
                "shared_with_platforms": [],
            },
        },
    ],
}


def _spec_with(**overrides: Any) -> dict[str, Any]:
    out = copy.deepcopy(_VALID_SPEC)
    out.update(overrides)
    return out


def _component(index: int, **overrides: Any) -> dict[str, Any]:
    out = copy.deepcopy(_VALID_SPEC["components"][index])
    out.update(overrides)
    return out


class HappyPathTest(unittest.TestCase):
    def test_baseline_valid_spec_returns_no_errors(self) -> None:
        errors = validate_spec_dict(_VALID_SPEC)
        self.assertEqual(errors, [])

    def test_single_instance_with_pcie_address_string_validates(self) -> None:
        spec = _spec_with()
        spec["components"] = [_component(1)]  # smb_fpga, pcie as string
        self.assertEqual(validate_spec_dict(spec), [])

    def test_multi_instance_at_end_validates(self) -> None:
        spec = _spec_with()
        spec["components"] = [_component(2)]  # pic_cpld with instances 1-2 at_end
        self.assertEqual(validate_spec_dict(spec), [])

    def test_multi_instance_before_first_underscore_validates(self) -> None:
        spec = _spec_with()
        comp = _component(2, name="pic_cpld")
        comp["instances"] = {"range": "1-2", "position": "before_first_underscore"}
        spec["components"] = [comp]
        self.assertEqual(validate_spec_dict(spec), [])

    def test_multi_instance_with_pcie_address_list_validates(self) -> None:
        spec = _spec_with()
        comp = _component(2)
        comp["pcie_address"] = ["01:00.0", "02:00.0"]
        spec["components"] = [comp]
        self.assertEqual(validate_spec_dict(spec), [])


class StructuralTest(unittest.TestCase):
    def test_missing_spec_version_fails(self) -> None:
        spec = _spec_with()
        del spec["spec_version"]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("[schema]" in e for e in errors))
        self.assertTrue(any("spec_version" in e for e in errors))

    def test_wrong_spec_version_fails(self) -> None:
        spec = _spec_with(spec_version="2.0")
        errors = validate_spec_dict(spec)
        self.assertTrue(any("spec_version" in e for e in errors))

    def test_silicon_must_be_list(self) -> None:
        spec = _spec_with()
        spec["platform"]["silicon"] = "TH6"
        errors = validate_spec_dict(spec)
        self.assertTrue(any("silicon" in e for e in errors))

    def test_silicon_empty_list_fails(self) -> None:
        spec = _spec_with()
        spec["platform"]["silicon"] = []
        errors = validate_spec_dict(spec)
        self.assertTrue(any("silicon" in e for e in errors))

    def test_silicon_missing_pcie_address_fails(self) -> None:
        spec = _spec_with()
        spec["platform"]["silicon"] = [{"name": "TH6"}]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("pcie_address" in e for e in errors))

    def test_silicon_bad_pcie_address_format_fails(self) -> None:
        spec = _spec_with()
        spec["platform"]["silicon"] = [{"name": "TH6", "pcie_address": "bad"}]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("pcie_address" in e for e in errors))

    def test_platform_name_uppercase_fails(self) -> None:
        spec = _spec_with()
        spec["platform"]["name"] = "ICECUBE800BC"
        errors = validate_spec_dict(spec)
        self.assertTrue(any("name" in e for e in errors))

    def test_priority_below_range_fails(self) -> None:
        spec = _spec_with()
        spec["components"][0]["firmware"]["priority"] = 0
        errors = validate_spec_dict(spec)
        self.assertTrue(any("priority" in e for e in errors))

    def test_priority_above_range_fails(self) -> None:
        spec = _spec_with()
        spec["components"][0]["firmware"]["priority"] = 101
        errors = validate_spec_dict(spec)
        self.assertTrue(any("priority" in e for e in errors))

    def test_invalid_position_value_fails(self) -> None:
        spec = _spec_with()
        spec["components"][2]["instances"]["position"] = "middle"
        errors = validate_spec_dict(spec)
        self.assertTrue(any("position" in e for e in errors))

    def test_pcie_address_bad_format_fails(self) -> None:
        spec = _spec_with()
        spec["components"][1]["pcie_address"] = "not-a-pcie-addr"
        errors = validate_spec_dict(spec)
        self.assertTrue(any("pcie_address" in e for e in errors))

    def test_unknown_top_level_field_fails(self) -> None:
        spec = _spec_with(unexpected_field="oops")
        errors = validate_spec_dict(spec)
        self.assertTrue(any("unexpected_field" in e for e in errors))

    def test_missing_firmware_block_fails(self) -> None:
        spec = _spec_with()
        del spec["components"][0]["firmware"]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("firmware" in e for e in errors))

    def test_missing_shared_with_platforms_fails(self) -> None:
        spec = _spec_with()
        del spec["components"][0]["firmware"]["shared_with_platforms"]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("shared_with_platforms" in e for e in errors))


class SemanticTest(unittest.TestCase):
    def test_before_first_underscore_without_underscore_fails(self) -> None:
        spec = _spec_with()
        spec["components"] = [
            _component(
                0,
                name="bios",
                instances={"range": "1-2", "position": "before_first_underscore"},
            )
        ]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("before_first_underscore" in e for e in errors))
        self.assertTrue(any("bios" in e for e in errors))

    def test_pcie_address_list_without_instances_fails(self) -> None:
        spec = _spec_with()
        comp = _component(1)
        comp["pcie_address"] = ["01:00.0", "02:00.0"]
        spec["components"] = [comp]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("pcie_address" in e for e in errors))

    def test_pcie_address_string_with_instances_fails(self) -> None:
        spec = _spec_with()
        comp = _component(2)
        comp["pcie_address"] = "01:00.0"
        spec["components"] = [comp]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("pcie_address" in e for e in errors))

    def test_pcie_address_list_length_mismatch_fails(self) -> None:
        spec = _spec_with()
        comp = _component(2)
        comp["pcie_address"] = ["01:00.0"]  # Range 1-2 → 2 instances expected
        spec["components"] = [comp]
        errors = validate_spec_dict(spec)
        self.assertTrue(any("pcie_address" in e for e in errors))
        self.assertTrue(any("instances" in e for e in errors))

    def test_post_expansion_name_collision_fails(self) -> None:
        spec = _spec_with()
        # `pic_cpld` with instances 1-2 expands to pic_cpld1 + pic_cpld2.
        # Add a second entry that literally collides with pic_cpld1.
        spec["components"].append(
            _component(0, name="pic_cpld1", display_location="pic")
        )
        errors = validate_spec_dict(spec)
        self.assertTrue(any("collision" in e for e in errors))
        self.assertTrue(any("pic_cpld1" in e for e in errors))

    def test_range_end_less_than_start_fails(self) -> None:
        spec = _spec_with()
        spec["components"][2]["instances"] = {"range": "5-2", "position": "at_end"}
        errors = validate_spec_dict(spec)
        # The schema's structural pattern accepts "5-2" syntactically;
        # tier 2 catches the end < start case.
        self.assertTrue(any("end" in e and "start" in e for e in errors))

    def test_range_zero_start_fails_at_schema(self) -> None:
        spec = _spec_with()
        spec["components"][2]["instances"] = {"range": "0-2", "position": "at_end"}
        errors = validate_spec_dict(spec)
        self.assertTrue(any("range" in e for e in errors))

    def test_range_malformed_fails(self) -> None:
        spec = _spec_with()
        spec["components"][2]["instances"] = {"range": "abc", "position": "at_end"}
        errors = validate_spec_dict(spec)
        self.assertTrue(any("range" in e for e in errors))

    def test_pcie_uppercase_hex_fails(self) -> None:
        # The schema mandates lowercase hex in BB:DD.F.
        spec = _spec_with()
        spec["components"][1]["pcie_address"] = "01:0A.0"
        errors = validate_spec_dict(spec)
        self.assertTrue(any("pcie_address" in e for e in errors))


class FieldExpansionTest(unittest.TestCase):
    """Verifies the effective_instance_names() expansion used in the orchestrator."""

    def test_no_instances_yields_single_name(self) -> None:
        comp = _Component.model_validate(_component(0))
        self.assertEqual(comp.effective_instance_names(), ["bios"])

    def test_at_end_expansion(self) -> None:
        comp = _Component.model_validate(_component(2))
        self.assertEqual(comp.effective_instance_names(), ["pic_cpld1", "pic_cpld2"])

    def test_before_first_underscore_expansion(self) -> None:
        comp_dict = _component(2)
        comp_dict["instances"] = {
            "range": "1-2",
            "position": "before_first_underscore",
        }
        comp = _Component.model_validate(comp_dict)
        self.assertEqual(comp.effective_instance_names(), ["pic1_cpld", "pic2_cpld"])


class SharedWithPlatformsCrossSpecTest(unittest.TestCase):
    """Tests for the cross-spec `firmware.shared_with_platforms` check that
    runs when `validate_spec_file(..., configs_root=...)` is called."""

    def _write_spec(self, configs_root: Path, platform: str, spec: Any) -> Path:
        platform_dir = configs_root / platform
        platform_dir.mkdir(parents=True, exist_ok=True)
        spec_path = platform_dir / "spec.json"
        spec_path.write_text(json.dumps(spec))
        return spec_path

    def test_missing_sibling_spec_emits_warning_only(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            configs_root = Path(tmp)
            spec = copy.deepcopy(_VALID_SPEC)
            spec["components"][0]["firmware"]["shared_with_platforms"] = [
                "not_yet_onboarded_platform",
            ]
            spec_path = self._write_spec(configs_root, "icecube800bc", spec)

            messages = validate_spec_file(spec_path, configs_root=configs_root)
            self.assertTrue(any(m.startswith("[warn]") for m in messages))
            self.assertFalse(any(m.startswith("[semantic]") for m in messages))
            self.assertFalse(any(m.startswith("[schema]") for m in messages))

    def test_matching_sibling_identifier_no_messages(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            configs_root = Path(tmp)
            sibling_spec = copy.deepcopy(_VALID_SPEC)
            sibling_spec["platform"]["name"] = "icecube800bd"
            # Same platform_identifier on the bios component → matches.
            sibling_spec["components"][0]["platform_identifier"] = "shared_bios_v1"
            self._write_spec(configs_root, "icecube800bd", sibling_spec)

            spec = copy.deepcopy(_VALID_SPEC)
            spec["components"][0]["platform_identifier"] = "shared_bios_v1"
            spec["components"][0]["firmware"]["shared_with_platforms"] = [
                "icecube800bd",
            ]
            spec_path = self._write_spec(configs_root, "icecube800bc", spec)

            messages = validate_spec_file(spec_path, configs_root=configs_root)
            self.assertEqual(messages, [])

    def test_mismatched_sibling_identifier_is_hard_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            configs_root = Path(tmp)
            sibling_spec = copy.deepcopy(_VALID_SPEC)
            sibling_spec["platform"]["name"] = "icecube800bd"
            sibling_spec["components"][0]["platform_identifier"] = "DIFFERENT_ID"
            self._write_spec(configs_root, "icecube800bd", sibling_spec)

            spec = copy.deepcopy(_VALID_SPEC)
            spec["components"][0]["platform_identifier"] = "shared_bios_v1"
            spec["components"][0]["firmware"]["shared_with_platforms"] = [
                "icecube800bd",
            ]
            spec_path = self._write_spec(configs_root, "icecube800bc", spec)

            messages = validate_spec_file(spec_path, configs_root=configs_root)
            hard = [m for m in messages if m.startswith("[semantic]")]
            self.assertEqual(len(hard), 1)
            self.assertIn("shared_bios_v1", hard[0])
            self.assertIn("DIFFERENT_ID", hard[0])
            self.assertIn("icecube800bd", hard[0])

    def test_sibling_missing_named_component_emits_warning(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            configs_root = Path(tmp)
            sibling_spec = copy.deepcopy(_VALID_SPEC)
            sibling_spec["platform"]["name"] = "icecube800bd"
            # Drop the bios component entirely from the sibling.
            sibling_spec["components"] = [
                c for c in sibling_spec["components"] if c["name"] != "bios"
            ]
            self._write_spec(configs_root, "icecube800bd", sibling_spec)

            spec = copy.deepcopy(_VALID_SPEC)
            spec["components"][0]["firmware"]["shared_with_platforms"] = [
                "icecube800bd",
            ]
            spec_path = self._write_spec(configs_root, "icecube800bc", spec)

            messages = validate_spec_file(spec_path, configs_root=configs_root)
            self.assertTrue(any(m.startswith("[warn]") for m in messages))
            self.assertFalse(any(m.startswith("[semantic]") for m in messages))

    def test_default_identifier_matches_platform_name_in_sibling(self) -> None:
        # Both specs omit platform_identifier → it defaults to platform.name on
        # each side. Different platform names → mismatched defaults → failure.
        with tempfile.TemporaryDirectory() as tmp:
            configs_root = Path(tmp)
            sibling_spec = copy.deepcopy(_VALID_SPEC)
            sibling_spec["platform"]["name"] = "icecube800bd"
            del sibling_spec["components"][0]["platform_identifier"]
            self._write_spec(configs_root, "icecube800bd", sibling_spec)

            spec = copy.deepcopy(_VALID_SPEC)
            del spec["components"][0]["platform_identifier"]
            spec["components"][0]["firmware"]["shared_with_platforms"] = [
                "icecube800bd",
            ]
            spec_path = self._write_spec(configs_root, "icecube800bc", spec)

            messages = validate_spec_file(spec_path, configs_root=configs_root)
            hard = [m for m in messages if m.startswith("[semantic]")]
            self.assertEqual(len(hard), 1)
            # The defaults are the platform names, so both should appear.
            self.assertIn("icecube800bc", hard[0])
            self.assertIn("icecube800bd", hard[0])

    def test_sibling_dir_name_differs_from_declared_platform_name(self) -> None:
        # Devmate-flagged regression: when a sibling's directory name does
        # NOT match its declared platform.name, the fallback for sibling's
        # platform_identifier must be the spec's platform.name (not the
        # directory name) — mirroring how own_identifier defaults to
        # spec's own platform.name.
        with tempfile.TemporaryDirectory() as tmp:
            configs_root = Path(tmp)
            # Sibling's spec.json declares platform.name="real_name" but
            # lives in a directory called "legacy_dir_name". Both sides
            # omit platform_identifier so they fall back to the declared
            # platform.name on each side.
            sibling_spec = copy.deepcopy(_VALID_SPEC)
            sibling_spec["platform"]["name"] = "real_name"
            del sibling_spec["components"][0]["platform_identifier"]
            self._write_spec(configs_root, "legacy_dir_name", sibling_spec)

            spec = copy.deepcopy(_VALID_SPEC)
            spec["platform"]["name"] = "real_name"
            del spec["components"][0]["platform_identifier"]
            spec["components"][0]["firmware"]["shared_with_platforms"] = [
                "legacy_dir_name",
            ]
            spec_path = self._write_spec(configs_root, "icecube800bc", spec)

            messages = validate_spec_file(spec_path, configs_root=configs_root)
            # Both sides default to platform.name="real_name" → identifiers
            # match → no [semantic] failure (would have failed if the
            # fallback used the directory name "legacy_dir_name" instead).
            hard = [m for m in messages if m.startswith("[semantic]")]
            self.assertEqual(hard, [])

    def test_malformed_sibling_spec_does_not_crash(self) -> None:
        # Devmate-flagged regression: sibling specs are NOT validated through
        # tier 1+2, so a malformed sibling (valid JSON but wrong shape — e.g.
        # `platform` is a string instead of an object, or `components` is
        # missing entirely) must NOT raise an unhandled AttributeError /
        # TypeError. The cross-spec check should emit a graceful [warn]
        # consistent with how it handles I/O and JSON parse failures.
        cases: list[Any] = [
            # Top-level is not even an object.
            ["not", "an", "object"],
            # `platform` is a string, not an object.
            {"platform": "icecube800bd", "components": []},
            # `components` is missing entirely.
            {"platform": {"name": "icecube800bd"}},
            # `components` is a string, not a list.
            {"platform": {"name": "icecube800bd"}, "components": "oops"},
            # `components` contains a non-dict entry.
            {
                "platform": {"name": "icecube800bd"},
                "components": ["not-a-dict"],
            },
            # `platform.name` is not a string — must NOT be cast and flow into
            # the identifier comparison (would emit a confusing [semantic]
            # mismatch instead of the intended graceful [warn]).
            {"platform": {"name": ["icecube800bd"]}, "components": []},
            {"platform": {"name": 42}, "components": []},
        ]
        for malformed_sibling in cases:
            with self.subTest(sibling=malformed_sibling):
                with tempfile.TemporaryDirectory() as tmp:
                    configs_root = Path(tmp)
                    self._write_spec(configs_root, "icecube800bd", malformed_sibling)
                    spec = copy.deepcopy(_VALID_SPEC)
                    spec["components"][0]["firmware"]["shared_with_platforms"] = [
                        "icecube800bd",
                    ]
                    spec_path = self._write_spec(configs_root, "icecube800bc", spec)

                    # Must not raise; must emit a [warn] for the malformed
                    # sibling and otherwise treat the own spec as valid.
                    messages = validate_spec_file(spec_path, configs_root=configs_root)
                    self.assertTrue(
                        any(m.startswith("[warn]") for m in messages),
                        f"expected [warn] for malformed sibling, got: {messages}",
                    )
                    hard = [
                        m for m in messages if m.startswith(("[schema]", "[semantic]"))
                    ]
                    self.assertEqual(
                        hard,
                        [],
                        f"unexpected hard failures from malformed sibling: {hard}",
                    )
