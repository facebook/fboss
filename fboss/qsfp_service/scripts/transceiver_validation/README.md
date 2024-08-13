# Transceiver Validation Scripts

Please use this scripts if you're interested in collecting data on unique transceiver configurations currently deployed in the FBOSS fleet. Included below are descriptions and use cases for each file.

## Typical Workflow

### Partial Fleet Query
Use this workflow if you want to query a specified number of random boxes across all tier and platform combinations. This is mainly meant to get a sample of the number of unique transceiver configurations without needing to query all boxes of a certain type.

1. Enter your desired parameters for the platforms and smc_tiers / roles you want to sample in `partial_fleet_query.sh`. Run the executable.
2. Run `python3 parse_tcvr_val_data.py` with `parse_all_platform_dumps(_DIR_PATH, _JSON_DIR_PATH)` in main.
3. Retrieve your desired JSON files in the output directory listed below.

### Platform Full Fleet Query (all boxes queried)
Use this workflow if you want to query all switches of a specific platform and smc_tier. Due to the number of switches, each switch's output will be written to a separate `.out` file, but will be compiled into one `.json`.

1. Enter your desired parameters into `role` and `platform` variables in `full_fleet_query.sh`. Run the executable.
2. Run `python3 parse_tcvr_val_data.py` with `parse_platform_dump(_DIR_PATH + {platform}, true, _JSON_DIR_PATH, {platform})` in main. The first `{platform}` argument should be the exact platform name specified in step 1. The second can be anything you prefer – it is just the name of the output file.
3. Retrieve your desired JSON file in the output directory listed below.

### Lab Device Query
Use this workflow if you want to query all lab devices of a set of platforms. We use this right now to determine all of the transceiver part numbers we currently test on.

1. Specify the platforms you want to query in `lab_device_scrape.sh`. Run the executable to run `sudo wedge_qsfp_util --direct-i2c` on all of a platform's alive switches in `fboss.qsfp.autotest`.
2. Run `ASAN_OPTIONS=detect_leaks=0 buck2 run fbcode//fboss/qsfp_service/scripts/transceiver_validation:generate_lab_configs` to parse the `wedge_qsfp_util` dumps and generate unique transceiver config files. We include the asan environment variable due to some special asan config for the fboss repository.

## Default Directories
`~/tcvr_configs/` – Home directory for raw command output (.out files).

`~/tcvr_configs_json/` – Home directory for parsed output (.json files).

`~/lab_configs/` – Home directory for raw command output (.out files) specifically for link test lab devices.

## Files
`partial_fleet_query.sh` – This shell script queries a custom number of boxes of every combination of role and platforms you specify. This script uses `netwhoami` randbox to select from a pool of valid switches based on the role and hardware attributes.

`full_fleet_query.sh` – This shell script queries all boxes of a certain role and platform across the entire FBOSS fleet. You can specify the role and platform you want to query the fleet for.

`parse_tcvr_val_data.py` – This file contains all logic for handling text dumps written by the above shell scripts. There are a few separate use cases:
- Parse and write JSONs for directory of different platforms: run the script normally.
- Parse and write JSONs for one specific file: use `parse_platform_dump()` directly.
- Parse and write JSONS for a directory containing files for one platform: use `parse_platform_dump()` with `is_multiple_files` as `True` and `raw_data_file_path` pointing to a directory.

`fetch_platform_lab_devices.py` – This file contains a small Python script for using a basset query to list all lab devices in the fboss.qsfp.autotest pool that are of a certain platform type. Each of these lab devices is then printed out on its own line, to be consumed by `lab_device_scrape.sh`. Each line contains the host name followed by hw_config as `platform,hw_config` (e.g. fboss32323232.snc1,5x16Q).

`lab_device_scrape.sh` – This file calls `sudo wedge_qsfp_util --direct-i2c` on all fboss.qsfp.autotest devices of all platforms listed in this file, and writes the output of each to a separate file to be parsed for later.

`generate_lab_configs.py` – This file processes all of the text dumps generated from `lab_device_scrape.sh` and generates files containing all unique transceiver configurations.
