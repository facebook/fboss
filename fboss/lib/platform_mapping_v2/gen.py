# pyre-strict
import os
from typing import Dict


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
