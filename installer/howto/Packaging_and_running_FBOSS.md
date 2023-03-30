# Packaging and running FBOSS HW tests on switch

## Requirements
 - FBOSS OSS binaries built as per the instructions in Building FBOSS OSS on Containers
 - HW switch running CentOS 8
 - ASIC sendor SDK kmods built as per the instructions from ASIC vendor and installed on the HW switch

## Packaging FBOSS

On container used for building FBOSS -

```
cd /var/FBOSS/fboss.git
./installer/centos-7-x86_64/package-fboss.py --scratch-path /var/FBOSS/tmp_bld_dir/
```

This creates a package directory with prefix /var/FBOSS/tmp_bld_dir/fboss_bins and copies all the essential artifacts of the FBOSS build process (binaries, dependencies, config files, helper scripts, etc.) from various locations to it.

This package directory could then be copied over to the test switch for exercising FBOSS and associated tests.

## Copy the directory to the switch

From the host VM of the container, copy over the FBOSS package directory to the switch -

```
scp -r /opt/app/FBOSS_DIR/tmp_bld_dir/fboss_bins-<pkg-id> root@$switchName:/opt/

On the switch, point /opt/fboss to the new package before using

```
ln -s  /opt/fboss_bins-<pkg-id> /opt/fboss
```

## Checking dependencies

Verify that all runtime dependencies are satisified for HW test binary including the libraries installed in /opt/fboss

```
cd /opt/fboss
source ./bin/setup_fboss_env
ldd /opt/fboss/bin/sai_test-sai_impl-1.11.0
```

If there are any missing libraries, then those need to be installed on the switch using "sudo dnf install ..." if the switch has internet access OR the missing libraries need to be copied from the FBOSS build's scratch path (/opt/app/FBOSS_DIR/tmp_bld_dir/installed/<missing_lib*>/) to switch (/opt/fboss/lib64/).

## Running FBOSS

Install SDK kmods as per the instructions from ASIC vendor

Switch to the FBOSS install directory on the switch and set up FBOSS environment

```
cd /opt/fboss
source ./bin/setup_fboss_env
```

Run setup script to populate fruid.json and other config files

```
./bin/setup.py
```

Running single HW test using HW test binary - 

```
./bin/sai_test-sai_impl-1.11.0 --config ./share/bcm_sai_configs/meru400bfu.agent.materialized_JSON --gtest_filter=HwFabricSwitchTest.init
```

Running multiple tests using test runner -

FBOSS tests can be launched by executing the test runner script. The script automatically selects and runs tests relevant to the BCM Wedge platform with optional filters.

```
./bin/run_test.py sai --sai-bin sai_test-sai_impl-1.11.0 --config ./share/bcm_sai_configs/meru400bfu.agent.materialized_JSON --gtest_filter=HwFabricSwitchTest.*
```

