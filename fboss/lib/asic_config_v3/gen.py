# pyre-strict

import json
import os
import sys

from fboss.lib.asic_config_v3.base_generator import BaseAsicConfigGenerator, MODULE_DIR
from fboss.lib.asic_config_v3.generators.broadcom_xgs_generator import (
    BroadcomXgsGenerator,
)

OUTPUT_DIR: str = f"{MODULE_DIR}/generated_asic_configs"

# Add a new (vendor, asic) entry when bringing up a new ASIC family.
_GENERATOR_REGISTRY: dict[tuple[str, str], type[BaseAsicConfigGenerator]] = {
    ("broadcom", "tomahawk5"): BroadcomXgsGenerator,
    ("broadcom", "tomahawk6"): BroadcomXgsGenerator,
}


def get_generator(
    platform_name: str, variant: str, platform_config: dict
) -> BaseAsicConfigGenerator:
    """Instantiate the correct generator based on vendor and ASIC."""
    vendor = platform_config["vendor"]
    asic = platform_config["asic"]
    key = (vendor, asic)
    generator_cls = _GENERATOR_REGISTRY.get(key)
    if not generator_cls:
        raise ValueError(f"No generator registered for vendor={vendor}, asic={asic}")
    return generator_cls(platform_name, variant, platform_config)


def discover_platforms() -> dict:
    """Return a mapping of platform name to platform config.

    Discovered by scanning ``platforms/*/asic_config.json``.
    """
    platforms_dir = os.path.join(MODULE_DIR, "platforms")
    platforms = {}

    if not os.path.exists(platforms_dir):
        return platforms

    for platform_name in os.listdir(platforms_dir):
        platform_path = os.path.join(platforms_dir, platform_name)
        if not os.path.isdir(platform_path):
            continue

        config_path = os.path.join(platform_path, "asic_config.json")
        if not os.path.exists(config_path):
            continue

        with open(config_path) as f:
            platform_config = json.load(f)
            platforms[platform_name] = platform_config

    return platforms


def generate_all_asic_configs() -> None:
    """Generate ASIC configs for every discovered platform and variant."""
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    for filename in os.listdir(OUTPUT_DIR):
        if filename.endswith((".json", ".yml")):
            os.remove(os.path.join(OUTPUT_DIR, filename))

    platforms = discover_platforms()

    for platform_name, platform_config in platforms.items():
        vendor = platform_config.get("vendor", "")
        asic = platform_config.get("asic", "")

        if (vendor, asic) not in _GENERATOR_REGISTRY:
            print(
                f"Skipping {platform_name} (no generator for vendor={vendor}, asic={asic})",
                file=sys.stderr,
            )
            continue

        variants = platform_config.get("variants", {})

        for variant_name in variants:
            print(
                f"Generating ASIC config for {platform_name}/{variant_name}...",
                file=sys.stderr,
            )

            try:
                generator = get_generator(platform_name, variant_name, platform_config)
                output = generator.generate()

                output_filename = (
                    f"{platform_name}_{variant_name}{generator.output_extension}"
                )
                output_path = os.path.join(OUTPUT_DIR, output_filename)

                print(f"Writing to {output_path}", file=sys.stderr)
                with open(output_path, "w", encoding="utf-8") as f:
                    f.write(output)

            except Exception as e:
                print(
                    f"Error generating config for {platform_name}/{variant_name}: {e}",
                    file=sys.stderr,
                )
                raise


if __name__ == "__main__":
    generate_all_asic_configs()
