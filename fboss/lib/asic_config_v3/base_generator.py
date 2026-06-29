# pyre-strict

import copy
import os
from abc import ABC, abstractmethod
from typing import Any

# Resolve config paths relative to the repo root rather than this file so the
# generator can run from the same checkout layout the bundled getdeps build
# expects.
_FBOSS_DIR: str = os.getcwd() + "/fboss"
MODULE_DIR: str = f"{_FBOSS_DIR}/lib/asic_config_v3"


def _deep_merge(base: dict[str, Any], override: dict[str, Any]) -> dict[str, Any]:
    """Recursively merge ``override`` on top of ``base``.

    Dict values present in both are merged recursively. For any other value
    type (scalars, lists, or a type mismatch between the two sides) the
    override replaces the base entry outright. Returns a new dict; the inputs
    are not mutated.
    """
    result = copy.deepcopy(base)
    for key, ov_value in override.items():
        if (
            key in result
            and isinstance(result[key], dict)
            and isinstance(ov_value, dict)
        ):
            result[key] = _deep_merge(result[key], ov_value)
        else:
            result[key] = copy.deepcopy(ov_value)
    return result


class BaseAsicConfigGenerator(ABC):
    """Abstract base class for ASIC config generators.

    Subclasses implement ``generate()`` to produce vendor-specific output
    such as YAML or JSON.
    """

    def __init__(
        self,
        platform_name: str,
        variant: str,
        platform_config: dict[str, Any],
    ) -> None:
        self.platform_name = platform_name
        self.variant = variant
        self.platform_config = platform_config

        vendor = platform_config.get("vendor")
        asic = platform_config.get("asic")
        if not vendor:
            raise ValueError("platform asic_config.json must define 'vendor'")
        if not asic:
            raise ValueError("platform asic_config.json must define 'asic'")
        self.asic_vendor: str = vendor
        self.asic_name: str = asic

        # The platform JSON may declare a top-level ``defaults`` block inherited
        # by every variant. The effective variant config is produced by deep-
        # merging the variant-specific entries on top of ``defaults``. Dict
        # values are merged recursively; scalars and lists are replaced.
        defaults = platform_config.get("defaults", {})
        variant_override = platform_config.get("variants", {}).get(variant, {})
        self.variant_config: dict[str, Any] = _deep_merge(defaults, variant_override)
        self.asic_config_params: dict[str, Any] = self.variant_config.get(
            "asic_config_params", {}
        )

    @abstractmethod
    def generate(self) -> str:
        """Generate the complete ASIC config and return it as a string."""
        ...

    @property
    @abstractmethod
    def output_extension(self) -> str:
        """File extension for the generated output (e.g. '.yml' or '.json')."""
        ...
