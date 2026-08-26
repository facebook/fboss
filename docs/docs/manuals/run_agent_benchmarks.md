---
id: run_agent_benchmarks
title: Run Agent Benchmark Tests
description: How to add run Agent benchmark tests
keywords:
    - FBOSS
    - OSS
    - agent
    - onboard
    - benchmark
    - manual
oncall: fboss_oss
---

## Overview

Running Agent benchmark tests consists of building the benchmark binaries and
then running them to observe the output.

If you run into issues, try checking our [troubleshooting guide](/docs/onboarding/troubleshooting/).

After each section, check the **Overall Outcomes** below to ensure everything is
completed.

## Overall Outcomes

### Setup

- an FBOSS forwarding stack Docker container to perform builds in

### Build and Test Agent Benchmarks

- benchmark binaries can be built
- benchmark binaries can be run

## Setup

### Step 1: Clone the FBOSS Repository

```bash file=./static/code_snips/clone_fboss.sh
```

### Step 2: Stop Existing Containers and Clean Docker Artifacts

```bash file=./static/code_snips/clean_docker.sh
```

### Step 3: Build the FBOSS Docker Image

```bash file=./static/code_snips/build_docker_image.sh
```

### Step 4: Start a Forwarding Stack Container

```bash file=./static/code_snips/start_forwarding_stack_container.sh
```

At this point, you should have a Docker container which can be used to build
the required artifacts in later steps.

## Build and Test Agent Benchmarks

### Step 1: Build Agent Benchmarks

Use the forwarding stack Docker container from the [Setup](./#setup-1) step.

#### Step 1.1: Enter the Container

```bash file=./static/code_snips/enter_docker_container.sh
```

#### Step 1.2: Build Agent Benchmark Binaries

The `run-getdeps.py` script accepts flags to configure the SAI implementation
and SDK version. Run `./fboss/oss/scripts/run-getdeps.py -h` to see all options
and Meta supported values. Note the `--benchmark-install` flag which is required
to install benchmark binaries.

```bash file=./static/code_snips/build_benchmark_binaries.sh
```

### Step 2: Send Build Artifacts to the Switch

#### Step 2.1: Package the Benchmark Binaries

```bash file=./static/code_snips/package_fboss.sh
```

#### Step 2.2: Copy and Set Up the Package

```bash file=./static/code_snips/copy_and_set_up_package.sh
```

### Step 3: Run Benchmark Tests

All benchmarks are consolidated into two binaries: `sai_all_benchmarks-sai_impl`
(mono, the default) and `sai_multi_switch_all_benchmarks-sai_impl` (multi-switch).
Individual benchmarks are selected at runtime via `--bm_regex`, with each test
running in its own process for full setup/run/teardown isolation.

#### Option 1: Using the run_test.py Script (Recommended)

The `run_test.py` script discovers all benchmarks from the binary via
`--bm_list`, pre-filters known-bad tests, then runs each test individually
with full setup/run/teardown isolation. Pass `--agent-run-mode multi_switch`
to use the multi-switch binary; mono is the default.

> The `benchmark` subcommand selects a benchmark family with mutually-exclusive
> `--sai` / `--qsfp` flags. This manual covers SAI agent benchmarks (`--sai`,
> the default when neither flag is given). For QSFP benchmarks (`--qsfp`), see
> the "Benchmark tests" section of
> [Packaging and Running FBOSS HW Tests](/docs/build/packaging_and_running_fboss_hw_tests_on_switch/#benchmark-tests).

```bash
cd /opt/fboss
source ./bin/setup_fboss_env

# Run all benchmarks (mono mode is the default)
./bin/run_test.py benchmark \
--config /opt/fboss/share/hw_test_configs/montblanc.agent.materialized_JSON \
--skip-known-bad-tests brcm/13.3.0.0_odp/tomahawk5 \
--test-run-timeout 1800

# Run all benchmarks against the multi-switch binary
./bin/run_test.py benchmark --agent-run-mode multi_switch \
--config /opt/fboss/share/hw_test_configs/montblanc.agent.materialized_JSON \
--skip-known-bad-tests brcm/13.3.0.0_odp/tomahawk5 \
--test-run-timeout 1800

# Run only benchmarks listed in a user-provided file (one test name per line)
./bin/run_test.py benchmark \
--config /opt/fboss/share/hw_test_configs/montblanc.agent.materialized_JSON \
--skip-known-bad-tests brcm/13.3.0.0_odp/tomahawk5 \
--filter_file /path/to/my_benchmarks.conf

# Run a single benchmark by name (regex match)
./bin/run_test.py benchmark \
--config /opt/fboss/share/hw_test_configs/montblanc.agent.materialized_JSON \
--skip-known-bad-tests brcm/13.3.0.0_odp/tomahawk5 \
--filter HwEcmpGroupShrink

# Run all route scale benchmarks
./bin/run_test.py benchmark \
--config /opt/fboss/share/hw_test_configs/montblanc.agent.materialized_JSON \
--skip-known-bad-tests brcm/13.3.0.0_odp/tomahawk5 \
--filter ".*Route.*Benchmark"

# List available benchmarks without running
./bin/run_test.py benchmark \
--config /opt/fboss/share/hw_test_configs/montblanc.agent.materialized_JSON \
--list_tests
```

**Benchmark-specific flags:**

| Flag | Description |
|------|-------------|
| `--sai` / `--qsfp` | Mutually exclusive benchmark-family selector. `--sai` (default) runs SAI agent benchmarks (this manual); `--qsfp` runs QSFP benchmarks (see Packaging and running FBOSS). |
| `--agent-run-mode` | `mono` (default) or `multi_switch`. Multi-switch passes `--multi_switch --hw_agent_for_testing` to the binary and manages the `hw_agent` service lifecycle automatically. |
| `--num-npus` | Number of NPUs for multi-switch mode (1 or 2, default 1). When 2, adds `--multi_npu_platform_mapping`. |
| `--filter` | Regex matched against discovered benchmark names to narrow which tests run. |
| `--filter_file` | File containing BENCHMARK() registered names to run (one per line). |
| `--skip-known-bad-tests` | Platform key (e.g. `brcm/13.3.0.0_odp/tomahawk5`) used both to pre-filter known-bad tests *before* running and to look up per-platform thresholds for result validation. Without it, all tests run and threshold validation is skipped (`NO_THRESHOLD`). |
| `--test-run-timeout` | Timeout for each test run in seconds. Default is 1200. |

When a benchmark has no threshold configured for the platform,
the runner prints a `WILL ALWAYS PASS` warning so missing entries are visible.

Results are written to a timestamped CSV file (e.g.,
`benchmark_results_20260119_143022.csv`) with these columns:

```
benchmark_binary_name, benchmark_test_name, benchmark_time_ps,
test_status, cpu_time_usec, max_rss, cpu_rx_pps, cpu_tx_pps,
threshold_status, threshold_details
```

#### Option 2: Running the Binary Directly

```bash
cd /opt/fboss
source ./bin/setup_fboss_env

# List all available benchmarks (no hardware init)
./bin/sai_all_benchmarks-sai_impl --bm_list

# Same for the multi-switch binary
./bin/sai_multi_switch_all_benchmarks-sai_impl --bm_list

# Run a single benchmark (mono)
./bin/sai_all_benchmarks-sai_impl --json \
--bm_regex "^HwEcmpGroupShrink$" \
--config ./share/hw_test_configs/<platform>.agent.materialized_JSON [--logging <level>]

# Run a single benchmark (multi-switch)
./bin/sai_multi_switch_all_benchmarks-sai_impl --json \
--multi_switch --hw_agent_for_testing \
--bm_regex "^HwEcmpGroupShrink$" \
--config ./share/hw_test_configs/<platform>.agent.materialized_JSON [--logging <level>]
```
