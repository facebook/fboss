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
./bin/sai_test-sai_impl-1.12.0 --config ./share/hw_test_configs/fuji.agent.materialized_JSON --filter=HwVoqSwitchWithFabricPortsTest.init
```

Running multiple tests using test runner:

FBOSS tests can be launched by executing the test runner script. The script automatically selects and runs tests relevant to the optional filters.
Also, the test runner can be run using various options - known good tests, known bad tests, include and exclude regexes, etc... These are documented
in run_test.py. After running all the tests, results will also be generated in a csv file.

```
./bin/run_test.py sai --sai-bin sai_test-sai_impl-1.12.0 --config ./share/hw_test_configs/fuji.agent.materialized_JSON --filter=HwVoqSwitchWithFabricPortsTest.*
```

## How to use run_test.py

The `run_test.py` script is a helper utility to simplify the process of running various types of FBOSS-related tests on a hardware device. It can be used to run the following types of tests:

- BCM tests
- SAI tests
- QSFP Hardware tests
- Link tests
- SAI Agent tests
- SAI Agent Scale tests
- SAI Agent Invariant tests
- Platform tests
- Benchmark tests

Examples of the command to run for each of the various types of tests are shown below.

**Important:** You must be in the `/opt/fboss` directory when running tests with `run_test.py`.

**Run mode — prefer `multi_switch`:** The agent-based test types (`sai_agent`, `sai_agent_scale`, `sai_invariant_agent`, `benchmark`) accept `--agent-run-mode mono|multi_switch`. Prefer **`multi_switch`** — it runs the SwSwitch control plane and the HwAgent (ASIC) as separate processes over Thrift, exactly as in production, and `run_test.py` manages the `hw_agent` service lifecycle for you. `mono` (single process) is deprecated and will be removed in the future. When `--agent-run-mode` is omitted these types default to `multi_switch` (except `benchmark`, whose historical default is `mono` — pass `--agent-run-mode multi_switch` explicitly).

### SAI tests

```
# Run sanity tests for Jericho2
./bin/run_test.py sai --config fuji.agent.materialized_JSON --coldboot_only --filter_file=/root/jericho2_sanity_tests

# Run sanity tests for Ramon
./bin/run_test.py sai --config fuji.agent.materialized_JSON --coldboot_only --filter_file=/root/ramon_sanity_tests

# Run entire BCM SAI XGS Regression for a specific ASIC type and SDK
./bin/run_test.py sai --config fuji.agent.materialized_JSON --skip-known-bad-tests "brcm/8.2.0.0_odp/8.2.0.0_odp/tomahawk4"

# Run entire SAI DNX regression for Jericho2 and SDK
./bin/run_test.py sai --config fuji.agent.materialized_JSON --skip-known-bad-tests "brcm/9.0_ea_dnx_odp/9.0_ea_dnx_odp/jericho2"
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
# Run LinkTest.asicLinkFlap for fuji using non-default platform mapping configs.
./bin/run_test.py link --config share/link_test_configs/fuji.materialized_JSON --qsfp-config /opt/fboss/share/qsfp_test_configs/fuji.materialized_JSON --filter=LinkTest.asicLinkFlap --platform_mapping_override_path /path/to/something --bsp_platform_mapping_override_path /path/to/something/else
```

Special flags:

1. `--filter`: FBOSS uses GTEST for it's test cases, and supports filtering tests via `--gtest_filter` ([doc](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests)). The filter is passed through to the test binary.
1. `--agent-run-mode`: the agent run mode to use. This value is passed through to the link tests. Currently it supports "mono" and "multi_switch" modes. If not specified, it will use "multi_switch" mode.
1. `--num-npus {1,2}`: Specify number of npus to run in multi switch mode. Default is 1.
1. `--bsp_platform_mapping_override_path`: an optional flag to override the BSP platform mapping. This value is passed through to the QSFP service binary.
1. `--platform_mapping_override_path`: an optional flag to override the ASIC platform mapping. This value is passed through to the QSFP service binary and the link tests binary.

### SAI Agent tests
```
# Run AgentRxReasonTests.InsertAndRemoveRxReason in multi_switch mode with num_npus = 1.
./bin/run_test.py sai_agent \
    --config ./share/hw_test_configs/$CONFIG \
    --filter AgentRxReasonTests.InsertAndRemoveRxReason --agent-run-mode multi_switch

# Warmboot soak: run a single test across many consecutive warmboots
# (coldboot once, then warmboot N times).
./bin/run_test.py sai_agent \
    --config ./share/hw_test_configs/$CONFIG --agent-run-mode multi_switch \
    --filter AgentEmptyTest.CheckInit --num-warmboot-iterations 64
```
1. `--filter`: FBOSS uses GTEST for it's test cases, and supports filtering tests via `--gtest_filter` ([doc](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests)). The filter is passed through to the test binary.
1. `--agent-run-mode`: the agent run mode to use. This value is passed through to the sai_agent tests. Currently it supports "mono" and "multi_switch" modes. If not specified, it will use "multi_switch" mode.
1. `--num-npus {1,2}`: Specify number of npus to run in multi switch mode. Default is 1.
1. `--num-warmboot-iterations N`: Run the test across N consecutive warmboots after the initial coldboot (default 1). Use with `--filter` to soak a single test (typically `AgentEmptyTest.CheckInit`) across many warmboots and catch warmboot-stability regressions. Each iteration re-arms warmboot so the next boot warmboots, and the soak stops early on the first non-passing iteration. Ignored when `--coldboot_only` is set. Applies to the agent test types (`sai`, `sai_agent`, `sai_agent_scale`, `sai_invariant_agent`).

### SAI Agent Scale tests

Scale tests stress FBOSS features at production-like resource counts (max ECMP groups/members, max ACL entries). See [Agent Scale Test](/docs/testing/agent_scale_test/).

```
# Run ALL scale tests (recommended). run_test.py discovers and runs every
# scale test; --skip-known-bad-tests skips tests known not to pass for this ASIC/SDK.
./bin/run_test.py sai_agent_scale \
    --config ./share/hw_test_configs/$CONFIG \
    --agent-run-mode multi_switch \
    --skip-known-bad-tests "brcm/13.3.0.0_odp/13.3.0.0_odp/tomahawk5/multi_switch"

# Run a single test (optional). The test name is for illustration only.
./bin/run_test.py sai_agent_scale \
    --config ./share/hw_test_configs/$CONFIG \
    --agent-run-mode multi_switch \
    --filter AgentEcmpTest.CreateMaxEcmpGroups
```

1. `--filter`: GTEST filter passed through to the test binary.
1. `--agent-run-mode`: `mono` or `multi_switch` (default `multi_switch`).
1. `--num-npus {1,2}`: number of NPUs to run in multi_switch mode (default 1).
1. `--skip-known-bad-tests`: ASIC/SDK key (`vendor/coldboot-sdk/warmboot-sdk/asic/mode`) to skip known-bad/unsupported tests. Scale tests reuse the SAI Agent known-bad lists.

### SAI Agent Invariant tests

Invariant tests verify core forwarding invariants (ACL, CoPP, load balancing, DSCP-to-queue, diag/Thrift handlers) hold after applying a production-like config. See [Agent Invariant Test](/docs/testing/agent_invariant_test/).

```
# Run ALL invariant tests (recommended). run_test.py discovers and runs every
# invariant test; --skip-known-bad-tests skips tests known not to pass for this ASIC/SDK.
./bin/run_test.py sai_invariant_agent \
    --config ./share/hw_test_configs/$CONFIG \
    --agent-run-mode multi_switch \
    --skip-known-bad-tests "brcm/13.3.0.0_odp/13.3.0.0_odp/tomahawk5/multi_switch"

# Run a single test (optional). The test name is for illustration only.
./bin/run_test.py sai_invariant_agent \
    --config ./share/hw_test_configs/$CONFIG \
    --agent-run-mode multi_switch \
    --filter ProdInvariantTest.verifyInvariants
```

1. `--filter`: GTEST filter passed through to the test binary. Use it to select a role variant, e.g. `ProdInvariantRtswTest.verifyInvariants`.
1. `--agent-run-mode`: `mono` or `multi_switch` (default `multi_switch`).
1. `--num-npus {1,2}`: number of NPUs to run in multi_switch mode (default 1).
1. `--skip-known-bad-tests`: ASIC/SDK key (`vendor/coldboot-sdk/warmboot-sdk/asic/mode`) to skip known-bad/unsupported tests. Invariant tests reuse the SAI Agent known-bad lists.

### Platform tests

```
# Run all Platform Services tests
./bin/run_test.py platform
```

Special flags:

1. `--filter`: FBOSS uses GTEST for its test cases, and supports filtering tests via `--gtest_filter` ([doc](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests)). The filter is passed through to the test binary.
1. `--type`: an optional flag to run specified platform service test (platform_hw_test, data_corral_service_hw_test, fan_service_hw_test, fw_util_hw_test, sensor_service_hw_test, weutil_hw_test, platform_manager_hw_test).

### Benchmark tests

The `benchmark` subcommand runs hardware benchmarks for two families, selected with mutually-exclusive flags: **SAI agent** benchmarks (`--sai`, the default when neither flag is given) and **QSFP** benchmarks (`--qsfp`). Both share one runner that discovers tests from the binary via `--bm_list`, pre-filters known-bad / unsupported tests, runs each test in its own process for full setup/run/teardown isolation, parses `--json` metrics, and validates them against per-platform thresholds.

#### SAI agent benchmarks

SAI benchmarks are consolidated into two binaries — `sai_all_benchmarks-sai_impl` (mono) and `sai_multi_switch_all_benchmarks-sai_impl` (multi-switch).

```
# Run all SAI benchmarks (--sai and mono are the defaults)
./bin/run_test.py benchmark --sai \
--config ./share/hw_test_configs/$CONFIG \
--skip-known-bad-tests brcm/13.3.0.0_odp/tomahawk5 \
--test-run-timeout 1800

# Run SAI benchmarks with the multi-switch binary
./bin/run_test.py benchmark --sai --agent-run-mode multi_switch \
--config ./share/hw_test_configs/$CONFIG \
--skip-known-bad-tests brcm/13.3.0.0_odp/tomahawk5 \
--test-run-timeout 1800
```

SAI-specific flags:

1. `--agent-run-mode`: Agent run mode to use. Supports `mono` (default, runs `sai_all_benchmarks-sai_impl`) and `multi_switch` (runs `sai_multi_switch_all_benchmarks-sai_impl`). In multi-switch mode the runner passes `--multi_switch --hw_agent_for_testing` to the binary and manages the `hw_agent` service lifecycle automatically.
1. `--num-npus`: Number of NPUs for multi-switch mode (1 or 2, default 1). When set to 2, `--multi_npu_platform_mapping` is passed to the binary.

SAI threshold validation looks up entries by benchmark name in `share/hw_benchmark_tests/sai_bench.materialized_JSON`. The matching mode (mono vs multi-switch) is preferred, falling back to the other mode if only one variant is configured.

#### QSFP benchmarks

QSFP benchmarks run from the `qsfp_hw_test_benchmark` binary, which self-initializes a WedgeManager in-process from `--qsfp-config` (no `qsfp_service`, `fsdb`, or `wedge_agent` needed) and forces its own coldboot per benchmark process.

```
# Run all QSFP benchmarks
./bin/run_test.py benchmark --qsfp \
--qsfp-config ./share/qsfp_test_configs/$QSFP_CONFIG \
--skip-known-bad-tests montblanc/credo-0.7.2 \
--test-run-timeout 1800

# Run a single QSFP benchmark
./bin/run_test.py benchmark --qsfp \
--qsfp-config ./share/qsfp_test_configs/$QSFP_CONFIG \
--skip-known-bad-tests montblanc/credo-0.7.2 \
--filter RefreshTransceiver_CWDM4_100G \
--test-run-timeout 1800
```

QSFP-specific flags:

1. `--qsfp-config`: Absolute path to the qsfp config (required with `--qsfp`).
1. `--force-5pim-fuji`: Force 5PIM Fuji mode (QSFP benchmarks on `fuji_5x16Q`).
1. `--port-manager-mode`: Enable port manager mode.

QSFP threshold validation looks up entries under the single buck-target key `//fboss/qsfp_service/test/benchmarks:qsfp_bench_test` in `share/hw_benchmark_tests/qsfp_bench.materialized_JSON`, selecting every entry whose `test_config_regex` matches the `<platform>/<phy_sdk>` key passed via `--skip-known-bad-tests`.

#### Common flags

1. `--sai` / `--qsfp`: Mutually exclusive; select the benchmark family. SAI is the default when neither is given.
1. `--filter`: Regex matched against discovered benchmark names to narrow which tests run.
1. `--filter_file`: File containing one `BENCHMARK()` registered name per line. To see the list, run the relevant binary with `--bm_list`.
1. `--skip-known-bad-tests`: Platform key used both to pre-filter known-bad tests *before* running and to look up per-platform thresholds for result validation. SAI uses `vendor/sdk/asic` (e.g. `brcm/13.3.0.0_odp/tomahawk5`); QSFP uses `<platform>/<phy_sdk>` (e.g. `montblanc/credo-0.7.2`). Without it, all discovered tests run and threshold validation is skipped (results show `NO_THRESHOLD`).
1. `--test-run-timeout`: Timeout for each test run. Default is 1200 seconds.

When no threshold is configured for a benchmark, the runner prints a `WILL ALWAYS PASS` warning so missing entries are visible. Results are written to a timestamped CSV (e.g. `benchmark_results_20260119_143022.csv`) with columns: `benchmark_binary_name, benchmark_test_name, benchmark_time_ps, test_status, cpu_time_usec, max_rss, cpu_rx_pps, cpu_tx_pps, threshold_status, threshold_details`.
