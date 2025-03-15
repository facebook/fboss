# Packaging and running FBOSS HW tests on switch

## Requirements
 - FBOSS OSS binaries built as per the instructions in Building FBOSS OSS on Containers
 - HW switch running CentOS 9
 - ASIC vendor SDK kmods built as per the instructions from ASIC vendor and installed on the HW switch

## Packaging FBOSS

On container used for building FBOSS -

```
cd /var/FBOSS/fboss.git
./fboss/oss/scripts/package-fboss.py --scratch-path /var/FBOSS/tmp_bld_dir/
```

This creates a package directory with prefix /var/FBOSS/tmp_bld_dir/fboss_bins and copies all the essential artifacts of the FBOSS build process (binaries, dependencies, config files, helper scripts, etc.) from various locations to it.

This package directory could then be copied over to the test switch for exercising FBOSS and associated tests.

The package directory can be compressed to create a tarball ("fboss_bins.tar.gz") using "--compress" option.

```
cd /var/FBOSS/fboss.git
./fboss/oss/scripts/package-fboss.py --scratch-path /var/FBOSS/tmp_bld_dir/ --compress
```

## Copy the directory or tarball to the switch

From the host VM of the container, copy over the FBOSS package directory or tarball to the switch -

```
scp -r /opt/app/FBOSS_DIR/tmp_bld_dir/fboss_bins-<$pkg-id> root@$switchName:/opt/ # directory copy
scp -r /opt/app/FBOSS_DIR/tmp_bld_dir/fboss_bins.tar.gz root@$switchName:/opt/ # tarball copy
```

On the switch, if tarball is copied, extract the tarball -

```
cd /opt/
tar -xzvf fboss_bins.tar.gz
```

On the switch, point /opt/fboss to the new package before using

```
ln -s  /opt/fboss_bins-<$pkg-id> /opt/fboss
```

## Checking dependencies

Verify that all runtime dependencies are satisified for HW test binary including the libraries installed in /opt/fboss

```
cd /opt/fboss
source ./bin/setup_fboss_env
ldd /opt/fboss/bin/sai_test-sai_impl-1.12.0
```

If there are any missing libraries, then those need to be installed on the switch using "sudo dnf install ..." if the switch has internet access OR the missing libraries need to be copied from the FBOSS build's scratch path (/opt/app/FBOSS_DIR/tmp_bld_dir/installed/<missing_lib*>/) to switch (/opt/fboss/lib64/).

## Running FBOSS

Install SDK kmods as per the instructions from ASIC vendor

Switch to the FBOSS install directory on the switch and set up FBOSS environment

```
cd /opt/fboss
source ./bin/setup_fboss_env
```

Run setup script to populate fruid.json and other config files (this step may not succeed on all platforms)

```
./bin/setup.py
```

Running single HW test using HW test binary -

```
./bin/sai_test-sai_impl-1.12.0 --config ./share/hw_test_configs/meru400biu.agent.materialized_JSON --filter=HwVoqSwitchWithFabricPortsTest.init
```

Running multiple tests using test runner -

FBOSS tests can be launched by executing the test runner script. The script automatically selects and runs tests relevant to the optional filters. Also, the test runner can be run using various options - known good tests, known bad tests, include and exclude regexes, etc... These are documented in run_test.py. After running all the tests, results will also be generated in a csv file.

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

Examples of the command to run for each of the various tyeps of tests are shown below.

### SAI tests

```
# Run sanity tests for Jericho2
./run_test.py sai --config meru400biu.agent.materialized_JSON --coldboot_only --filter_file=/root/jericho2_sanity_tests

# Run sanity tests for Ramon
./run_test.py sai --config meru400bfu.agent.materialized_JSON --coldboot_only --filter_file=/root/ramon_sanity_tests

# Run entire BCM SAI XGS Regression for a specific ASIC type and SDK
./run_test.py sai --config fuji.agent.materialized_JSON --skip-known-bad-tests "brcm/8.2.0.0_odp/8.2.0.0_odp/tomahawk4"

# Run entire SAI DNX regression for Jericho2 and SDK
./run_test.py sai --config meru400biu.agent.materialized_JSON --skip-known-bad-tests "brcm/9.0_ea_dnx_odp/9.0_ea_dnx_odp/jericho2"
```

### QSFP Hardware tests

```
# Run all QSFP HW tests
./run_test.py qsfp --qsfp-config $qsfpConfFile

# Run EmptyHwTest.CheckInit using non-default platform mapping configs.
./run_test.py qsfp --qsfp-config ./test_qsfp_config_current --filter=EmptyHwTest.CheckInit --bsp_platform_mapping_override_path /fake/bad/path --platform_mapping_override_path /another/bad/path
```

Special flags:

1. `--filter`: FBOSS uses GTEST for it's test cases, and supports filtering tests via `--gtest_filter` ([doc](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests)). The filter is passed through to the test binary.
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
1. `--platform_mapping_override_path`: an optioanl flag to override the ASIC platform mapping. This value is passed through to the QSFP service binary and the link tests binary.
