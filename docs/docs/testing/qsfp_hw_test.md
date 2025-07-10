# QSFP Hardware Tests

## Overview

QSFP hardware tests are a set of unit tests designed to verify the functionality of `qsfp_service` on a physical switch.

The QSFP hardware tests cover various aspects of the transceivers, including:

- I2C interface communication
- Control pins functionality
- Transceiver management

## Test Categories

1. **I2C Interface**
    - **Read/Write stress test to known registers**: This test verifies that the I2C interface can read and write data to specific registers on the QSFP transceiver.

2. **Control Pins**
    - The control pins on the QSFP transceivers are used to manage their operation, including resetting the transceiver or putting it into a low-power state.
    - The control pins test category includes:
        - **Presence**: This test ensures that the QSFP transceiver can be detected by `qsfp_service`.
        - **Reset**: Each test starts in cold-boot mode, asserting and de-asserting the reset pin of the transceiver to verify its functionality.

3. **Transceiver Management**
    - The transceiver management test category includes:
        - Verifies application advertisements
        - Media interface detection
        - VDM (Versatile Data Management) capabilities advertisements
        - Transceiver datapath programming: These tests program the speed provided in the config and verifies that the datapath programming completes successfully.
        - CDB Firmware Upgrade

## Vendor Utilization

- **Platform Vendor**: Should verify BSP (Board Support Package) kmods for the platform using the `qsfp_hw_test`.
- **Optics Vendor**: Should verify the Transceiver Management Interface, I2C interface, and control pins to transceivers using the `qsfp_hw_test`.

## Pre-requisites

- BSP Kmods loaded
- Switch cabled per QSFP/Link Test Topology guidelines
- QSFP Test Config created per cabled topology
- Compiled QSFP hardware tests
    - Follow the instructions on the [build page](/docs/build/building_fboss_on_docker_containers).
    - When running the build step, you can pass this optional flag to save time: `--cmake-target qsfp_hw_test`

## Command to Run QSFP Hardware Tests

Refer to this [documentation](/docs/build/packaging_and_running_fboss_hw_tests_on_switch/#qsfp-hardware-tests).
