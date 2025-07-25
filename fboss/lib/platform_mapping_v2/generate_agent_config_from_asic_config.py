import argparse
import json


def load_yaml_file(file_path):
    try:
        with open(file_path, "r") as f:
            content = f.read()
            return content.encode("ascii", errors="replace").decode("ascii")
    except FileNotFoundError:
        print("File not found.")
        return None


def save_to_json(data, json_file_path, new_file_path):
    try:
        with open(json_file_path, "r+") as f:
            json_data = json.load(f)
            if (
                "platform" in json_data
                and "chip" in json_data["platform"]
                and "asicConfig" in json_data["platform"]["chip"]
                and "common" in json_data["platform"]["chip"]["asicConfig"]
            ):
                json_data["platform"]["chip"]["asicConfig"]["common"]["yamlConfig"] = (
                    data
                )
                with open(new_file_path, "w") as fnew:
                    json.dump(json_data, fnew, indent=4)
            else:
                print("JSON structure does not match expected format.")
    except FileNotFoundError:
        print("JSON file not found.")
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON: {e}")


def main():
    parser = argparse.ArgumentParser(
        epilog="""
        This python script embeds the asic yaml config file into the agent json config file. Usage Example:
        python3 generate_agent_config_from_asic_config.py -agent_config icecube_configuration/montblanc.agent.materialized_JSON -asic_config icecube_configuration/bcm78914_b0-generic-128x800.config.yml -output_config "/tmp/icepack.agent.materialized_JSON"
    """
    )
    parser.add_argument(
        "-agent_config",
        dest="agentConfigFilePath",
        type=str,
        help="path to agent json config file",
    )
    parser.add_argument(
        "-asic_config",
        dest="asicConfigFilePath",
        type=str,
        help="path to asic yaml config file",
    )
    parser.add_argument(
        "-output_config",
        dest="agentOutputConfigFilePath",
        type=str,
        help="path to output agent json config file",
    )

    args = parser.parse_args()

    yaml_data = load_yaml_file(args.asicConfigFilePath)
    if yaml_data:
        save_to_json(
            yaml_data, args.agentConfigFilePath, args.agentOutputConfigFilePath
        )


if __name__ == "__main__":
    main()
