# pyre-strict
import argparse
import json
import os
import sys
from typing import Any, Dict, List, Optional, Tuple, Union

from fboss.lib.platform_mapping_v2.platform_mapping_v2 import PlatformMappingV2
from fboss.lib.platform_mapping_v2.read_files_utils import (
    _FBOSS_DIR,
    INPUT_DIR,
    read_all_vendor_data,
    read_platform_descriptor,
)
from thrift.python.serializer import Protocol, serialize

JsonValue = Union[Dict[str, Any], List[Any], str, int, float, bool, None]
PlatformDescriptorData = Tuple[str, Dict[str, Any]]


def _is_thrift_map(d: object) -> bool:
    """Return True when *d* looks like a serialized Thrift map (all-numeric keys)."""
    return isinstance(d, dict) and bool(d) and all(k.isdigit() for k in d)


def _format_json(obj: JsonValue) -> str:
    """Format *obj* as JSON matching the old TSimpleJSONProtocol indentation."""
    return _dump(obj, depth=0, extra=0)


def _dump(
    obj: JsonValue,
    depth: int,
    extra: int,
    close_extra: Optional[int] = None,
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
        parts: List[str] = []
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


def get_command_line_args() -> Tuple[str, str, str, bool]:
    parser = argparse.ArgumentParser(description="OSS platform mapping generation.")
    parser.add_argument(
        "--input-dir",
        type=str,
        default=INPUT_DIR,  # temporary location until platform name is read in
        required=False,
        help="Path to input directory holding CSVs (default: fboss/lib/platform_mapping_v2/platforms/PLATFORM_NAME)",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default=f"{_FBOSS_DIR}/lib/platform_mapping_v2/generated_platform_mappings/",
        required=False,
        help="Path to output directory for JSON (default: fboss/lib/platform_mapping_v2/generated_platform_mappings/)",
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
    return (
        args.input_dir + args.platform_name,
        args.output_dir,
        args.platform_name,
        args.multi_npu,
    )


def generate_platform_mappings(
    input_dir: str, output_dir: str, platform_name: str, is_multi_npu: bool
) -> None:
    print(f"Finding vendor data in {input_dir}...", file=sys.stderr)
    input_dir = os.path.expanduser(input_dir)
    vendor_data_map = read_all_vendor_data()

    if not vendor_data_map:
        print("No vendor data found in the input directory.", file=sys.stderr)
        exit(1)

    print("Generating platform mapping...", file=sys.stderr)
    platform_mapping = PlatformMappingV2(
        vendor_data_map, platform_name, is_multi_npu
    ).get_platform_mapping()

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

    generate_platform_descriptor(
        vendor_data_map, output_dir, platform_name, platform_descriptor_data
    )


def get_platform_descriptor_data(
    vendor_data_map: Dict[str, Dict[str, str]], platform_name: str
) -> Optional[PlatformDescriptorData]:
    try:
        platform_descriptor = read_platform_descriptor(
            vendor_data_map[platform_name], platform_name
        )
    except FileNotFoundError:
        return None

    system_vendor = platform_descriptor.pop("systemVendor")
    if not isinstance(system_vendor, str):
        raise TypeError(f"Invalid system vendor for {platform_name}")

    return (system_vendor, platform_descriptor)


def generate_platform_descriptor(
    vendor_data_map: Dict[str, Dict[str, str]],
    output_dir: str,
    platform_name: str,
    platform_descriptor_data: Optional[PlatformDescriptorData] = None,
) -> None:
    if platform_descriptor_data is None:
        platform_descriptor_data = get_platform_descriptor_data(
            vendor_data_map, platform_name
        )
    if platform_descriptor_data is None:
        return

    _, platform_descriptor = platform_descriptor_data
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
