# FBOSS BSP Mapping Config Generation

## Introduction

This document describes how to generate and consume Board Support Package (BSP) mapping configuration for FBOSS platforms.

The BSP mapping JSON is built into FBOSS binaries via per-platform `*BspPlatformMapping` C++ classes for platforms that use BSP mapping, such as `MontblancBspPlatformMapping`. For each module (PIM/FRU) in the system, it stores:
- the transceiver reset/presence and I2C controller settings
- the LED mapping for transceivers
- MDIO and reset settings for external PHYs (when present)

Vendors are expected to provide CSV input files, run the BSP mapping generator to produce JSON, and then embed that JSON into the appropriate C++ mapping files.


## Source File Specification

### BSP Mapping

- **File name pattern:** `PLATFORM_BspMapping.csv`
- **Location:** `fboss/lib/bsp/bspmapping/input/`

This CSV file contains transceiver access, reset/presence, IO controller, and LED mapping information. Each row describes a set of lanes for a single transceiver and optionally a single LED.

Example: [`fboss/lib/bsp/bspmapping/input/Montblanc_BspMapping.csv`](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/bspmapping/input/Montblanc_BspMapping.csv).


### Optional PHY BSP Mapping

- **File name pattern:** `PLATFORM_PhyBspMapping.csv`
- **Location:** `fboss/lib/bsp/bspmapping/input/`

This CSV file is optional. When present, it describes the mapping between external PHYs and the MDIO controllers that manage them, including reset GPIO paths.

By convention, the PHY BSP mapping CSV name replaces the `_BspMapping.csv` suffix with `_PhyBspMapping.csv`. For example, for `Ladakh800bcls_BspMapping.csv` the corresponding PHY file is `Ladakh800bcls_PhyBspMapping.csv` in the same directory.

Example: [`fboss/lib/bsp/bspmapping/input/Ladakh800bcls_PhyBspMapping.csv`](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/bspmapping/input/Ladakh800bcls_PhyBspMapping.csv).


## CSV Column Definitions

### Transceiver Mapping CSV (`PLATFORM_BspMapping.csv`)

The transceiver mapping CSV must have a header row and the following columns, in this order:

| Column Name          | Definition |
|----------------------|------------|
| **TcvrId**           | ID of the transceiver within the PIM. |
| **TcvrLaneIdList**   | Space-separated list of logical lane IDs within the transceiver. |
| **PimId**            | ID of the PIM/FRU this transceiver belongs to (1 for a fixed system). |
| **AccessControllerId** | Identifier for the reset/presence controller (e.g. a CPLD instance). Rows sharing this ID use the same controller. |
| **AccessControlType** | Type of reset/presence controller. Supported values: `CPLD`, `GPIO`. |
| **ResetPath**        | Device path used to control the transceiver reset line, for the given access controller. |
| **ResetMask**        | Bit mask within the reset register that controls this transceiver. |
| **ResetHoldHi**      | Reset polarity: 1 = active‑high, 0 = active‑low. |
| **PresentPath**      | Device path used to read the transceiver presence signal. |
| **PresentMask**      | Bit mask within the presence register that corresponds to this transceiver. |
| **PresentHoldHi**    | Presence polarity: 1 = active‑high, 0 = active‑low. |
| **IoControllerId**   | Identifier of the I2C controller used to talk to this transceiver. |
| **IoControlType**    | Type of IO controller. Supported value today: `I2C`. |
| **IoPath**           | Device path for the IO controller (e.g. `/run/devmap/xcvrs/xcvr_io_1`). |
| **LedId**            | (Optional) ID of the LED associated with the lanes in `TcvrLaneIdList`. |
| **LedBluePath**      | (Optional) Device path for the blue or green portion of the LED. |
| **LedYellowPath**    | (Optional) Device path for the yellow or amber portion of the LED. |

Notes:

- The first row is treated as a header and skipped by the parser.
- Only Lane IDs 1–8 are permitted in `TcvrLaneIdList`.
- `TcvrLaneIdList`, `LedId`, `LedBluePath` and `LedYellowPath` may be empty.
- Device path is usually PlatformManager device map path (e.g. `/run/devmap/xcvrs/xcvr_io_1`), but it can also be a sysfs path.


### PHY Mapping CSV (`PLATFORM_PhyBspMapping.csv`)

The optional PHY mapping CSV must have a header row and the following columns, in this order:

| Column Name             | Definition |
|-------------------------|------------|
| **PhyId**               | ID of the external PHY. |
| **PhyCoreId**           | ID of the core within the PHY. |
| **PimId**               | ID of the PIM/FRU this PHY belongs to. |
| **PhyResetPath**        | Device path used to control the reset line for this PHY. |
| **IoControlType**       | Type of IO controller used to talk to this PHY. Supported value today: `MDIO`. |
| **IoControllerId**      | ID of the MDIO controller instance. |
| **IoControllerResetPath** | Device path used to control reset for the MDIO controller. |
| **IoPath**              | Device path for the MDIO bus. |
| **PhyAddr**             | MDIO address of the PHY on the bus. |

Notes:

- If `PLATFORM_PhyBspMapping.csv` is not present for a platform, the generated BSP mapping will contain only transceiver and LED information.
- Device path is usually PlatformManager device map path (e.g. `/run/devmap/xcvrs/xcvr_io_1`), but it can also be a sysfs path.


## BSP Mapping Generation Workflow

### BSP Mapping JSON Generation Tool

#### Prerequisites

Python 3 is required in order to run the helper script. You can install it via one of the below commands depending on which Linux distribution you're on.

##### Debian

```shell
sudo apt update
sudo apt -y upgrade
sudo apt install python3
```

##### CentOS

```shell
sudo dnf upgrade -y
sudo dnf install python3
```

#### Instructions

Generate the BSP Mapping configuration by running the helper shell script `run-helper.sh` from the root of the FBOSS repository:

```shell
$ ./fboss/lib/bsp/bspmapping/run-helper.sh
```

This runs the `fboss-bspmapping-gen` binary for each platform registered in the internal hardware map and writes one JSON file per platform to `generated_configs` in the system temporary directory, typically `/tmp/generated_configs` on Linux.

The JSON file names are derived from the platform type and lowercased. For example:

- `MONTBLANC` → `generated_configs/montblanc.json`
- `JANGA800BIC` → `generated_configs/janga800bic.json`
- `MERU400BFU` → `generated_configs/meru400bfu.json`
- `ICECUBE` → `generated_configs/icecube.json`
- `ICETEA` → `generated_configs/icetea.json`


### Adding or Updating a BSP Mapping for a Platform

To add or update BSP mapping for a platform `PLATFORM`:

1. **Create/update CSV input files**
   - Add or update `fboss/lib/bsp/bspmapping/input/PLATFORM_BspMapping.csv`.
   - (Optional) If the platform has external PHYs managed over MDIO, also add `fboss/lib/bsp/bspmapping/input/PLATFORM_PhyBspMapping.csv`.

2. **Register the platform CSV in the BSP mapping generator**
   - Edit [`fboss/lib/bsp/bspmapping/Parser.h`](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/bspmapping/Parser.h) and add a new constant for the platform CSV, following the existing pattern:

     ```c++
     inline constexpr folly::StringPiece kPortMapping<PlatformName>Csv{
         "PLATFORM_BspMapping.csv"};
     ```

      `PlatformName` should be replaced with the new platform name being added, for example:
     ```c++
     inline constexpr folly::StringPiece kPortMappingMontblancCsv{
         "Montblanc_BspMapping.csv"};
     ```

   - Edit [`fboss/lib/bsp/bspmapping/Main.cpp`](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/bspmapping/Main.cpp) and add an entry to `kHardwareNameMap` that maps the appropriate `PlatformType` to the new CSV constant, again following the existing pattern.

   - Edit [`fboss/lib/bsp/bspmapping/test/ParserTest.cpp`](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/bspmapping/test/ParserTest.cpp) to add the new platform to `ParserTest.GetNameForTests`.

3. **Generate BSP mapping JSON**
   - From the FBOSS repository root, run:

     ```shell
     ./fboss/lib/bsp/bspmapping/run-helper.sh
     ```

   - After the script completes, locate the generated JSON file under `/tmp/generated_configs/`.

4. **Embed JSON into the per-platform C++ mapping class**
   - Each platform has a corresponding `*BspPlatformMapping` C++ class under `fboss/lib/bsp/PLATFORM/`, for example:
     - [`fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.cpp`](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.cpp)
     - [`fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.cpp`](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.cpp)
   - Open the `*BspPlatformMapping.cpp` file and replace the `kJsonBspPlatformMappingStr` string literal with the contents of the newly generated JSON file.
   - Rebuild FBOSS and re-run any relevant tests.

Note: Unlike Platform Mapping, BSP Mapping currently does not have a dedicated `verify_generated_files.py` script. Only the CSV input files are committed to the repository; the embedded JSON string in each `*BspPlatformMapping.cpp` file must be kept in sync with those CSVs manually.


### JSON File to C++ File Mapping

For convenience, the table below shows the mapping between generated JSON files and the corresponding C++ BSP platform mapping classes for some of the existing platforms:

| JSON File Name                     | C++ File Location                                                     |
|------------------------------------|------------------------------------------------------------------------|
| generated_configs/janga800bic.json | fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.cpp           |
| generated_configs/ladakh800bcls.json | fboss/lib/bsp/ladakh800bcls/Ladakh800bclsBspPlatformMapping.cpp     |
| generated_configs/meru800bfa.json  | fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.cpp             |
| generated_configs/minipack3n.json  | fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.cpp             |
| generated_configs/montblanc.json   | fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.cpp               |
| generated_configs/morgan800cc.json | fboss/lib/bsp/morgan800cc/Morgan800ccBspPlatformMapping.cpp           |
| generated_configs/tahan800bc.json  | fboss/lib/bsp/tahan800bc/Tahan800bcBspPlatformMapping.cpp             |
| generated_configs/wedge800bact.json | fboss/lib/bsp/wedge800bact/Wedge800BACTBspPlatformMapping.cpp       |
