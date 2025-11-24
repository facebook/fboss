---
id: packaging_and_running_fboss_hw_tests_on_switch
title: Packaging and Running FBOSS HW Tests on Switch
description: How to package and run FBOSS HW tests
keywords:
    - FBOSS
    - OSS
    - build
    - docker
    - test
    - HW
oncall: fboss_oss
---

## Requirements
 - FBOSS OSS binaries built per the instructions in the [build guide](/docs/build/building_fboss_on_docker_containers/)
 - HW switch running CentOS 9
 - ASIC vendor SDK kmods built as per the instructions from ASIC vendor and installed on the HW switch

## Package FBOSS

The first step is to package FBOSS. We have a script that creates a package directory with all essential artifacts of the FBOSS build process.
Things like binaries, dependencies, config files, helper scripts, and more will be copied from various locations into this directory.

It can also be compressed to create a tarball for ease of use.

```bash file=./static/code_snips/package_fboss.sh
```

This package directory or tarball can now be copied over to the test switch for exercising FBOSS and associated tests.

## Copy the Directory or Tarball to the Switch

```bash file=./static/code_snips/copy_and_set_up_package.sh
```

## Check Dependencies

```bash file=./static/code_snips/check_dependencies.sh
```

## Run FBOSS

First, install SDK kmods as per the instructions from ASIC vendor.

Run the setup script to populate fruid.json and other config files (this step may not succeed on all platforms):

```
cd /opt/fboss
./bin/setup.py
```

We recommend using the test runner to run tests, but you may also run tests using the binary
directly. This is an example of running a single HW test using the HW test binary:

```
./bin/sai_test-sai_impl-1.12.0 --config ./share/hw_test_configs/meru400biu.agent.materialized_JSON --filter=HwVoqSwitchWithFabricPortsTest.init
```

Running multiple tests using test runner:

FBOSS tests can be launched by executing the test runner script. The script automatically selects and runs tests relevant to the optional filters.
Also, the test runner can be run using various options - known good tests, known bad tests, include and exclude regexes, etc... These are documented
in run_test.py. After running all the tests, results will also be generated in a csv file.

```
./bin/run_test.py sai --sai-bin sai_test-sai_impl-1.12.0 --config ./share/hw_test_configs/meru400biu.agent.materialized_JSON --filter=HwVoqSwitchWithFabricPortsTest.*
```

## How to use run_test.py

The `run_test.py` script is a helper utility to simplify the process of running various types of FBOSS-related tests on a hardware device. It can be used to run the following types of tests:

- BCM tests
- SAI tests
- QSFP Hardware tests
- Link tests
- SAI Agent tests

Examples of the command to run for each of the various types of tests are shown below.

**Important:** You must be in the `/opt/fboss` directory when running tests with `run_test.py`.

### SAI tests

```
# Run sanity tests for Jericho2
./bin/run_test.py sai --config meru400biu.agent.materialized_JSON --coldboot_only --filter_file=/root/jericho2_sanity_tests

# Run sanity tests for Ramon
./bin/run_test.py sai --config meru400bfu.agent.materialized_JSON --coldboot_only --filter_file=/root/ramon_sanity_tests

# Run entire BCM SAI XGS Regression for a specific ASIC type and SDK
./bin/run_test.py sai --config fuji.agent.materialized_JSON --skip-known-bad-tests "brcm/8.2.0.0_odp/8.2.0.0_odp/tomahawk4"

# Run entire SAI DNX regression for Jericho2 and SDK
./bin/run_test.py sai --config meru400biu.agent.materialized_JSON --skip-known-bad-tests "brcm/9.0_ea_dnx_odp/9.0_ea_dnx_odp/jericho2"
```

### QSFP Hardware tests

```
# Run all QSFP HW tests
./bin/run_test.py qsfp --qsfp-config $qsfpConfFile

# Run EmptyHwTest.CheckInit using non-default platform mapping configs
./bin/run_test.py qsfp --qsfp-config ./test_qsfp_config_current --filter=EmptyHwTest.CheckInit --bsp_platform_mapping_override_path /fake/bad/path --platform_mapping_override_path /another/bad/path
```

Special flags:

1. `--filter`: FBOSS uses GTEST for its test cases, and supports filtering tests via `--gtest_filter` ([doc](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests)). The filter is passed through to the test binary.
1. `--bsp_platform_mapping_override_path`: an optional flag to override the BSP platform mapping. This value is passed through to the QSFP HW test binary.
1. `--platform_mapping_override_path`: an optioanl flag to override the ASIC platform mapping. This value is passed through to the QSFP HW test binary.

### Link tests

```
# Run LinkTest.asicLinkFlap for meru400bia using non-default platform mapping configs.
# NOTE: We recommend using mono mode to run link tests on a single ASIC platform.
./bin/run_test.py link --agent-run-mode mono --config share/link_test_configs/meru400bia.materialized_JSON --qsfp-config /opt/fboss/share/qsfp_test_configs/meru400bia.materialized_JSON --filter=LinkTest.asicLinkFlap --platform_mapping_override_path /path/to/something --bsp_platform_mapping_override_path /path/to/something/else
```

Special flags:

1. `--filter`: FBOSS uses GTEST for it's test cases, and supports filtering tests via `--gtest_filter` ([doc](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests)). The filter is passed through to the test binary.
1. `--agent-run-mode`: the agent run mode to use. This value is passed through to the link tests. Currently it supports "mono" and "legacy" modes. If not specified, it will use "legacy" mode.
1. `--bsp_platform_mapping_override_path`: an optional flag to override the BSP platform mapping. This value is passed through to the QSFP service binary.
1. `--platform_mapping_override_path`: an optional flag to override the ASIC platform mapping. This value is passed through to the QSFP service binary and the link tests binary.
