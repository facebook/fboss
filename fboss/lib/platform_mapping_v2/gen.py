# pyre-strict
import argparse
import json
import os
import sys
from typing import Any, Dict, List, Optional, Tuple, Union

from fboss.lib.platform_mapping_v2.platform_mapping_v2 import PlatformMappingV2
from thrift.python.serializer import Protocol, serialize

_FBOSS_DIR: str = os.getcwd() + "/fboss"
INPUT_DIR: str = f"{_FBOSS_DIR}/lib/platform_mapping_v2/platforms/"

JsonValue = Union[Dict[str, Any], List[Any], str, int, float, bool, None]


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


def read_vendor_data(input_file_path: str) -> Dict[str, str]:
    vendor_data = {}
    if not os.path.exists(input_file_path):
        raise FileNotFoundError(f"The folder '{input_file_path}' does not exist.")

    for filename in os.listdir(input_file_path):
        filepath = os.path.join(input_file_path, filename)
        if (
            filepath.endswith(".csv") or filepath.endswith(".json")
        ) and not os.path.isdir(filepath):
            with open(filepath, "r") as file:
                content = file.read()
            vendor_data[filename] = content

    return vendor_data


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
    os.makedirs(output_dir, exist_ok=True)
    platform_file_name = f"{platform_name}_platform_mapping" + (
        "_is_multi_npu" if is_multi_npu else ""
    )
    output_file = f"{output_dir}/{platform_file_name}.json"
    platform_mapping_serialized = serialize(platform_mapping, protocol=Protocol.JSON)
    platform_mapping_json = _format_json(json.loads(platform_mapping_serialized))

    # Ensure that files end in a newline (we have pre-commit hooks in OSS that
    # will fail if this isn't done.)
    if not platform_mapping_json.endswith("\n"):
        platform_mapping_json += "\n"

    print(f"Writing to file {output_file}...", file=sys.stderr)
    with open(os.path.expanduser(output_file), "w") as f:
        f.write(platform_mapping_json)


def generate_mappings_without_args() -> None:
    generate_platform_mappings(*get_command_line_args())


def read_all_vendor_data() -> Dict[str, Dict[str, str]]:
    all_vendor_data = {}
    data_path = INPUT_DIR
    print(
        f"Reading all vendor data in {data_path}...",
        file=sys.stderr,
    )
    for filename in os.listdir(data_path):
        filepath = os.path.join(data_path, filename)
        if not os.path.isdir(filepath):
            continue
        all_vendor_data[filename] = read_vendor_data(filepath)

    return all_vendor_data


if __name__ == "__main__":
    generate_mappings_without_args()
