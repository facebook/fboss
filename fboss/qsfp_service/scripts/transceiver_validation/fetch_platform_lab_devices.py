import json
import subprocess
import sys

"""
print_basset_devices: Takes in a command line argument for the platform whose fboss.qsfp.autotest lab devices you want to retrieve. Print a new-line delimited list of all switch hostnames and hardware configurations (e.g. fboss32323232.snc1,5x16Q).
"""


def print_basset_devices():
    if len(sys.argv) <= 1:
        return
    platform = sys.argv[1]
    cmd = ["basset", "list", f"fboss.qsfp.autotest:{platform}", "--raw"]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = process.communicate()

    parsed_output = json.loads(output)
    all_devices = []
    for metadata in parsed_output:
        if metadata["status"] == 6:
            # Skipping over dead devices.
            continue
        device = metadata["name"]
        hw_config = ""
        for attr in metadata["attributes"]:
            if attr["name"] == "hardware_configuration":
                hw_config = attr["value"]

        all_devices.append(f"{device},{hw_config}")

    print("\n".join(all_devices))


if __name__ == "__main__":
    print_basset_devices()
