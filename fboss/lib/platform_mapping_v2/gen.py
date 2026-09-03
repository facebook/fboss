# pyre-strict
import argparse
import json
import os
import sys
from dataclasses import dataclass
from typing import Any, Optional, Union

from fboss.lib.platform_mapping_v2.platform_mapping_v2 import PlatformMappingV2
from fboss.lib.platform_mapping_v2.read_files_utils import (
    discover_platform_mapping_inputs,
    PlatformMappingInputs,
    read_platform_descriptor,
)
from neteng.fboss.platform_config.platform_config.thrift_types import (
    PortIdToPortAssignmentConfig,
)
from thrift.python.serializer import Protocol, serialize

JsonValue = Union[dict[str, Any], list[Any], str, int, float, bool, None]
PlatformDescriptorData = tuple[str, dict[str, Any]]


@dataclass(frozen=True)
class PlatformMappingPaths:
    fboss_root: str
    input_dir: str
    output_dir: str

    @classmethod
    def from_root(
        cls,
        fboss_root: str,
        input_dir: Optional[str] = None,
        output_dir: Optional[str] = None,
    ) -> "PlatformMappingPaths":
        root = os.path.abspath(os.path.expanduser(fboss_root))

        def resolve(path: str) -> str:
            return os.path.abspath(os.path.expanduser(path))

        return cls(
            fboss_root=root,
            input_dir=resolve(input_dir or os.path.join(root, "configs", "platforms")),
            output_dir=resolve(
                output_dir
                or os.path.join(
                    root, "lib", "platform_mapping_v2", "generated_platform_mappings"
                )
            ),
        )


_RAW_PLATFORM_MAPPING_FAMILIES: dict[str, tuple[str, ...]] = {
    "icecube800bc": ("icecube800bc",),
    "montblanc": ("montblanc", "montblanc_odd_ports_8x100G", "montblanc_gtsw_yolo"),
    "tahansb800bc": ("tahansb800bc", "tahansb800bc_test_fixture"),
    "wedge800bact": ("wedge800bact", "wedge800bnhp"),
    "wedge800cact": ("wedge800cact",),
}


def _is_thrift_map(d: object) -> bool:
    """Return True when *d* looks like a serialized Thrift map (all-numeric keys)."""
    return isinstance(d, dict) and bool(d) and all(k.isdigit() for k in d)


def _format_json(obj: JsonValue) -> str:
    """Format *obj* as JSON matching the old TSimpleJSONProtocol indentation."""
    return _dump(obj, depth=0, extra=0)


def _dump(
    obj: JsonValue, depth: int, extra: int, close_extra: Optional[int] = None
) -> str:
    """Recursively serialize *obj* to a JSON string.

    ``depth`` controls the base 2-space indentation level.  ``extra`` adds
    additional spaces — TSimpleJSONProtocol indents map-value structs by an
    extra 2 spaces per nesting level.  ``close_extra`` (defaulting to
    ``extra``) sets the indent for the closing brace/bracket; inside a map
    the child content is indented by ``extra + 2`` but the closing delimiter
    reverts to the parent's ``extra``.
    """
    if close_extra is None:
        close_extra = extra
    indent = "  " * depth + " " * extra
    close_indent = "  " * depth + " " * close_extra

    if isinstance(obj, dict):
        if not obj:
            return "{}"
        parts: list[str] = []
        is_map = _is_thrift_map(obj)
        for i, (k, v) in enumerate(obj.items()):
            comma = "," if i < len(obj) - 1 else ""
            # Map values get extra indent; close_extra stays at parent level.
            child_extra = extra + 2 if is_map else extra
            v_str = _dump(v, depth + 1, child_extra, extra)
            parts.append(f"{indent}  {json.dumps(k)}: {v_str}{comma}")
        return "{\n" + "\n".join(parts) + "\n" + close_indent + "}"

    if isinstance(obj, list):
        if not obj:
            # TSimpleJSONProtocol renders empty lists as multi-line.
            return "[\n" + indent + "  " + "\n" + close_indent + "]"
        parts = []
        for i, v in enumerate(obj):
            comma = "," if i < len(obj) - 1 else ""
            v_str = _dump(v, depth + 1, extra)
            parts.append(f"{indent}  {v_str}{comma}")
        return "[\n" + "\n".join(parts) + "\n" + close_indent + "]"

    return json.dumps(obj)


def get_command_line_args() -> tuple[str, str, str, bool]:
    parser = argparse.ArgumentParser(description="OSS platform mapping generation.")
    parser.add_argument(
        "--fboss-root",
        type=str,
        required=True,
        help=(
            "Path to the fboss/ source directory itself, for example "
            "/path/to/fbcode/fboss."
        ),
    )
    parser.add_argument(
        "--input-dir",
        type=str,
        default=None,
        required=False,
        help=(
            "Path to the directory containing platform input directories. "
            "When omitted, uses FBOSS_ROOT/configs/platforms."
        ),
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default=None,
        required=False,
        help=(
            "Path to the output directory for JSON. When omitted, uses "
            "FBOSS_ROOT/lib/platform_mapping_v2/generated_platform_mappings."
        ),
    )
    parser.add_argument(
        "--platform-name",
        type=str,
        required=True,
        help="Name of platform that each CSV file has as prefix",
    )
    parser.add_argument(
        "--multi-npu",
        action="store_true",
        help="Generate multi-NPU platform mapping (default: False)",
    )

    args = parser.parse_args()
    paths = PlatformMappingPaths.from_root(
        args.fboss_root, args.input_dir, args.output_dir
    )
    return (paths.input_dir, paths.output_dir, args.platform_name, args.multi_npu)


def generate_platform_mappings(
    input_dir: str, output_dir: str, platform_name: str, is_multi_npu: bool
) -> None:
    print(f"Finding vendor data in {input_dir}...", file=sys.stderr)
    input_dir = os.path.expanduser(input_dir)
    vendor_data_map = discover_platform_mapping_inputs(input_dir)
    generate_platform_mappings_from_vendor_data(
        vendor_data_map, output_dir, platform_name, is_multi_npu
    )


def generate_platform_mappings_from_vendor_data(
    vendor_data_map: PlatformMappingInputs,
    output_dir: str,
    platform_name: str,
    is_multi_npu: bool,
) -> None:
    if not vendor_data_map:
        print("No vendor data found in the input directory.", file=sys.stderr)
        sys.exit(1)

    print("Generating platform mapping...", file=sys.stderr)
    generator = PlatformMappingV2(vendor_data_map, platform_name, is_multi_npu)
    platform_mapping = generator.get_platform_mapping()

    output_dir = os.path.expanduser(output_dir)
    platform_descriptor_data = get_platform_descriptor_data(
        vendor_data_map, platform_name
    )
    if platform_descriptor_data is not None:
        system_vendor, _ = platform_descriptor_data
        output_dir = f"{output_dir}/{system_vendor}/{platform_name}"
        output_file = f"{output_dir}/platform_mapping.json"
    else:
        platform_file_name = f"{platform_name}_platform_mapping" + (
            "_is_multi_npu" if is_multi_npu else ""
        )
        output_file = f"{output_dir}/{platform_file_name}.json"

    os.makedirs(output_dir, exist_ok=True)
    platform_mapping_serialized = serialize(platform_mapping, protocol=Protocol.JSON)
    platform_mapping_json = _format_json(json.loads(platform_mapping_serialized))

    # Ensure that files end in a newline (we have pre-commit hooks in OSS that
    # will fail if this isn't done.)
    if not platform_mapping_json.endswith("\n"):
        platform_mapping_json += "\n"

    print(f"Writing to file {output_file}...", file=sys.stderr)
    with open(os.path.expanduser(output_file), "w") as f:
        f.write(platform_mapping_json)

    write_raw_platform_mapping_artifacts(
        vendor_data_map, output_dir, platform_name, is_multi_npu, generator
    )

    generate_platform_descriptor(
        vendor_data_map,
        output_dir,
        platform_name,
        generator.get_num_switch_asics(),
        platform_descriptor_data,
    )


def _get_raw_platform_mapping_family(platform_name: str) -> Optional[tuple[str, ...]]:
    for base_platform, family in _RAW_PLATFORM_MAPPING_FAMILIES.items():
        if platform_name == base_platform or platform_name in family:
            return family
    return None


def _serialize_thrift(value: Any) -> str:
    serialized = serialize(value, protocol=Protocol.JSON)
    formatted = _format_json(json.loads(serialized))
    return formatted if formatted.endswith("\n") else formatted + "\n"


def _serialize_port_assignments(generator: PlatformMappingV2) -> str:
    assignments = {
        str(port_id): json.loads(serialize(assignment, protocol=Protocol.JSON))
        for port_id, assignment in generator.get_port_id_to_port_assignment().items()
    }
    formatted = _format_json({"portIdToPortAssignment": assignments})
    return formatted if formatted.endswith("\n") else formatted + "\n"


def write_raw_platform_mapping_artifacts(
    vendor_data_map: PlatformMappingInputs,
    output_dir: str,
    platform_name: str,
    is_multi_npu: bool,
    generator: Optional[PlatformMappingV2] = None,
) -> Optional[PortIdToPortAssignmentConfig]:
    family = _get_raw_platform_mapping_family(platform_name)
    if family is None:
        return None

    family_mappings = [
        PlatformMappingV2(
            vendor_data_map, family_platform, is_multi_npu
        ).get_platform_mapping()
        for family_platform in family
    ]
    raw_mapping = PlatformMappingV2.generate_raw_platform_mapping(family_mappings)
    if generator is None:
        generator = PlatformMappingV2(vendor_data_map, platform_name, is_multi_npu)

    port_id_to_port_assignment = PortIdToPortAssignmentConfig(
        portIdToPortAssignment=generator.get_port_id_to_port_assignment()
    )

    raw_mapping_file = os.path.join(output_dir, "raw_platform_mapping.json")
    assignment_file = os.path.join(output_dir, "port_id_to_port_assignment.json")
    print(f"Writing to file {raw_mapping_file}...", file=sys.stderr)
    with open(raw_mapping_file, "w") as f:
        f.write(_serialize_thrift(raw_mapping))
    print(f"Writing to file {assignment_file}...", file=sys.stderr)
    with open(assignment_file, "w") as f:
        f.write(_serialize_port_assignments(generator))
    return port_id_to_port_assignment


def get_platform_descriptor_data(
    vendor_data_map: PlatformMappingInputs, platform_name: str
) -> Optional[PlatformDescriptorData]:
    try:
        platform_descriptor = read_platform_descriptor(
            vendor_data_map[platform_name].data, platform_name
        )
    except FileNotFoundError:
        return None

    system_vendor = platform_descriptor.pop("systemVendor")
    if not isinstance(system_vendor, str):
        raise TypeError(f"Invalid system vendor for {platform_name}")

    return (system_vendor, platform_descriptor)


def generate_platform_descriptor(
    vendor_data_map: PlatformMappingInputs,
    output_dir: str,
    platform_name: str,
    num_switch_asics: int,
    platform_descriptor_data: Optional[PlatformDescriptorData] = None,
) -> None:
    if platform_descriptor_data is None:
        platform_descriptor_data = get_platform_descriptor_data(
            vendor_data_map, platform_name
        )
    if platform_descriptor_data is None:
        return
    if num_switch_asics < 1:
        raise ValueError(
            f"Static mapping for {platform_name} does not define a switch ASIC"
        )

    _, platform_descriptor = platform_descriptor_data
    platform_descriptor = dict(platform_descriptor)
    platform_descriptor["numSwitchAsics"] = num_switch_asics
    descriptor_json = _format_json(platform_descriptor)
    if not descriptor_json.endswith("\n"):
        descriptor_json += "\n"

    os.makedirs(output_dir, exist_ok=True)
    output_file = f"{output_dir}/platform_descriptor.json"

    print(f"Writing to file {output_file}...", file=sys.stderr)
    with open(output_file, "w") as f:
        f.write(descriptor_json)


def generate_mappings_without_args() -> None:
    generate_platform_mappings(*get_command_line_args())


if __name__ == "__main__":
    generate_mappings_without_args()
