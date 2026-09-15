# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import annotations

import argparse
import fcntl
import json
import os
import pathlib
import stat
import tempfile
from collections.abc import Mapping
from datetime import datetime, timezone


READ_ONLY_MODE = stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH


def _read_metadata(path: pathlib.Path) -> dict[str, object]:
    if not path.exists():
        return {}
    contents = json.loads(path.read_text())
    if not isinstance(contents, dict):
        raise ValueError(f"NPU SDK metadata must be a JSON object: {path}")
    return contents


def _write_metadata(path: pathlib.Path, metadata: Mapping[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_path = tempfile.mkstemp(
        dir=path.parent,
        prefix=f".{path.name}.",
        text=True,
    )
    try:
        with os.fdopen(descriptor, "w") as output:
            json.dump(metadata, output, indent=2, sort_keys=True)
            output.write("\n")
            output.flush()
            os.fsync(output.fileno())
        os.chmod(temporary_path, READ_ONLY_MODE)
        os.replace(temporary_path, path)
    finally:
        if os.path.exists(temporary_path):
            os.unlink(temporary_path)


def _update_metadata(
    path: pathlib.Path,
    binary_name: str,
    entry: Mapping[str, object] | None,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lock_path = path.with_name(f".{path.name}.lock")
    with lock_path.open("a+") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        metadata = _read_metadata(path)
        if entry is None:
            if binary_name not in metadata:
                return
            del metadata[binary_name]
        else:
            metadata[binary_name] = dict(entry)

        if metadata:
            _write_metadata(path, metadata)
        elif path.exists():
            path.unlink()


def record_binary_metadata(
    path: pathlib.Path,
    binary_name: str,
    npu_sai_impl: str,
    npu_sai_sdk_selector: str,
    asic_sdk_version: str,
    sai_sdk_version: str,
    built_at: str | None = None,
) -> None:
    _update_metadata(
        path,
        binary_name,
        {
            "npuSaiImpl": npu_sai_impl,
            "npuSaiSdkSelector": npu_sai_sdk_selector,
            "sdkVersion": {
                "asicSdk": asic_sdk_version,
                "saiSdk": sai_sdk_version,
            },
            "builtAt": built_at
            or datetime.now(timezone.utc)
            .isoformat(timespec="seconds")
            .replace("+00:00", "Z"),
        },
    )


def remove_binary_metadata(path: pathlib.Path, binary_name: str) -> None:
    _update_metadata(path, binary_name, None)


def _create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Record NPU SDK metadata after an SDK-linked target builds."
    )
    subparsers = parser.add_subparsers(dest="operation", required=True)

    record = subparsers.add_parser("record")
    record.add_argument("--metadata-path", type=pathlib.Path, required=True)
    record.add_argument("--binary-name", required=True)
    record.add_argument("--npu-sai-impl", required=True)
    record.add_argument("--npu-sai-sdk-selector", required=True)
    record.add_argument("--asic-sdk-version", required=True)
    record.add_argument("--sai-sdk-version", required=True)

    remove = subparsers.add_parser("remove")
    remove.add_argument("--metadata-path", type=pathlib.Path, required=True)
    remove.add_argument("--binary-name", required=True)

    return parser


def main() -> None:
    args = _create_parser().parse_args()
    if args.operation == "record":
        record_binary_metadata(
            path=args.metadata_path,
            binary_name=args.binary_name,
            npu_sai_impl=args.npu_sai_impl,
            npu_sai_sdk_selector=args.npu_sai_sdk_selector,
            asic_sdk_version=args.asic_sdk_version,
            sai_sdk_version=args.sai_sdk_version,
        )
    else:
        remove_binary_metadata(args.metadata_path, args.binary_name)


if __name__ == "__main__":
    main()
