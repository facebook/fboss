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
 - FBOSS OSS binaries built per the instructions in Building FBOSS OSS on Docker Containers
 - HW switch running CentOS 9
 - ASIC vendor SDK kmods built as per the instructions from ASIC vendor and installed on the HW switch

## Package FBOSS

The first step is to package FBOSS. We have a script that creates a package directory with all essential artifacts of the FBOSS build process.
Things like binaries, dependencies, config files, helper scripts, and more will be copied from various locations into this directory.

It can also be compressed to create a tarball for ease of use.

On the container used for building FBOSS:

```
# Navigate to the fboss repository
cd /var/FBOSS/fboss

# Creates a package directory with prefix /var/FBOSS/tmp_bld_dir/fboss_bins
./fboss/oss/scripts/package-fboss.py --copy-root-libs --scratch-path /var/FBOSS/tmp_bld_dir/

# Creates a tarball called "fboss_bins.tar.zst" under /var/FBOSS/tmp_bld_dir/
./fboss/oss/scripts/package-fboss.py --copy-root-libs --scratch-path /var/FBOSS/tmp_bld_dir/ --compress
```

This package directory or tarball can now be copied over to the test switch for exercising FBOSS and associated tests.

## Copy the Directory or Tarball to the Switch

From the host VM of the container, use the appropriate command to copy the FBOSS package directory or tarball to the switch:

```
# Copy the directory
scp -r /opt/app/FBOSS_DIR/tmp_bld_dir/fboss_bins-<$pkg-id> root@<$switchName>:/opt/

# Copy the tarball
scp /opt/app/FBOSS_DIR/tmp_bld_dir/fboss_bins.tar.zst root@<$switchName>:/opt/
```

Then set up the package on the switch:

```
cd /opt

# If using directory
ln -s /opt/fboss_bins-<$pkg-id> /opt/fboss

# If using tarball
mkdir fboss && mv fboss_bins.tar.zst fboss/
cd fboss && tar -xvf fboss_bins.tar.zst
```

## Check Dependencies

Set up FBOSS environment variables:

```
cd /opt/fboss
source ./bin/setup_fboss_env
```

Verify that all runtime dependencies are satisified for the test binary including the libraries installed in /opt/fboss:

```
ldd /opt/fboss/bin/<$binaryName>

# Ensure you dont see any 'not found'. Example:
$ ldd /opt/fboss/bin/sai_test-fake
        ...
        # Good
        libcurl.so.4 => /lib64/libcurl.so.4 (0x00007f3f26d39000)
        libyaml-0.so.2 => /opt/fboss/lib/libyaml-0.so.2 (0x00007f3f26d18000)
        ...
        # Bad
        libre2.so.9 => not found
        libsodium.so.23 => not found
        ...
```

If there are any missing libraries, then those need to be installed on the switch using "sudo dnf install ..." if the switch has internet access. Alternatively,
the missing libraries can be copied from the FBOSS build's scratch path `/opt/app/FBOSS_DIR/tmp_bld_dir/installed/<missing_lib*>/` to switch `/opt/fboss/lib/`.

## Run FBOSS

First, install SDK kmods as per the instructions from ASIC vendor.

Run the setup script to populate fruid.json and other config files (this step may not succeed on all platforms):

```
cd /opt/fboss
./bin/setup.py
```

Running single HW test using HW test binary:

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
./bin/run_test.py link --config share/link_test_configs/meru400bia.materialized_JSON --qsfp-config share/qsfp_test_configs/meru400bia.materialized_JSON --filter=LinkTest.asicLinkFlap --platform_mapping_override_path /path/to/something --bsp_platform_mapping_override_path /path/to/something/else
```

Special flags:

1. `--filter`: FBOSS uses GTEST for it's test cases, and supports filtering tests via `--gtest_filter` ([doc](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests)). The filter is passed through to the test binary.
1. `--bsp_platform_mapping_override_path`: an optional flag to override the BSP platform mapping. This value is passed through to the QSFP service binary.
1. `--platform_mapping_override_path`: an optional flag to override the ASIC platform mapping. This value is passed through to the QSFP service binary and the link tests binary.
