# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict
import json
import subprocess
from typing import Any, Dict, List

"""
An API which calls the conveyor command and then parses the results into objects that can be used by the CLI to display stuff to the user
"""


def get_release_status(
    conveyor_id: str,
    release_instance: str,
) -> List[Dict[str, Any]]:
    """
    Get release status from conveyor CLI

    Args:
        conveyor_id: name of the conveyor
        release_instance: R1112.3
    """
    cmd = [
        "conveyor",
        "release",
        "status",
        "--conveyor-id",
        conveyor_id,
        "--release-instance",
        release_instance,
        "--json",
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)

        # Find where the JSON array starts
        json_start = result.stdout.find("[")
        if json_start == -1:
            return []

        # Extract just the JSON portion (from [ to the end of stdout)
        json_text = result.stdout[json_start:]

        # Parse the JSON
        data = json.loads(json_text)
        return data
    except Exception as e:
        print(
            "Oops, there was a problem while getting the json from the conveyor cli:", e
        )
    return []
