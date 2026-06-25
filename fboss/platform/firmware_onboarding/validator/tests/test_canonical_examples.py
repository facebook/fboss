# pyre-strict

"""Validates every canonical per-platform `spec.json` under
`fboss/platform/firmware_onboarding/configs/<platform>/`. Catches regressions
where a schema or validator change breaks an existing onboarded spec.

When a new platform is onboarded:
1. Add an entry to `_CANONICAL_PLATFORMS` below.
2. Add a corresponding entry to the `resources` dict in
   `//fboss/platform/firmware_onboarding/validator:test_canonical_examples`.
"""

from __future__ import annotations

import json
import unittest
from importlib.resources import files

from fboss.platform.firmware_onboarding.validator.spec_validator import (
    validate_spec_dict,
)


_CANONICAL_PLATFORMS: list[str] = [
    "icecube800bc",
]


class CanonicalExamplesTest(unittest.TestCase):
    def test_each_canonical_spec_validates(self) -> None:
        package = __package__ or "fboss.platform.firmware_onboarding.validator.tests"
        for platform in _CANONICAL_PLATFORMS:
            with self.subTest(platform=platform):
                resource = files(package) / f"{platform}.json"
                spec_obj = json.loads(resource.read_text())
                errors = validate_spec_dict(spec_obj)
                self.assertEqual(
                    errors,
                    [],
                    f"{platform}/spec.json failed validation: {errors}",
                )
