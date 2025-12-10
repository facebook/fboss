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

#### Step 1.2: Run the Build Helper

```bash file=./static/code_snips/build_helper.sh
```

#### Step 1.3: Set Important Environment Variables

```bash file=./static/code_snips/important_environment_variables.sh
```

For Agent benchmark binaries, you also need to set another env variable:

```bash
export BENCHMARK_INSTALL=1
```

#### Step 1.4: Build Agent Benchmark Binaries

Remember to set the `BENCHMARK_INSTALL` env variable from the previous step.

```bash file=./static/code_snips/build_forwarding_stack.sh
```

### Step 2: Send Build Artifacts to the Switch

#### Step 2.1: Package the Benchmark Binaries

```bash file=./static/code_snips/package_fboss.sh
```

#### Step 2.2: Copy and Set Up the Package

```bash file=./static/code_snips/copy_and_set_up_package.sh
```

### Step 3: Run Benchmark Binaries

Run the relevant binaries that can be found in the `bin` directory on the
switch:

```bash
cd /opt/fboss
source ./bin/setup_fboss_env

./bin/sai_fsw_scale_route_add_speed-sai_impl
./bin/sai_hgrid_du_scale_route_add_speed-sai_impl
./bin/sai_hgrid_uu_scale_route_del_speed-sai_impl
./bin/sai_th_alpm_scale_route_add_speed-sai_impl
./bin/sai_fsw_scale_route_del_speed-sai_impl
./bin/sai_ecmp_shrink_with_competing_route_updates_speed-sai_impl
./bin/sai_th_alpm_scale_route_del_speed-sai_impl
./bin/sai_ecmp_shrink_speed-sai_impl
./bin/sai_hgrid_uu_scale_route_add_speed-sai_impl
./bin/sai_hgrid_du_scale_route_del_speed-sai_impl
./bin/sai_stats_collection_speed-sai_impl
./bin/sai_tx_slow_path_rate-sai_impl
./bin/sai_rx_slow_path_rate-sai_impl
./bin/sai_init_and_exit_40Gx10G-sai_impl
./bin/sai_init_and_exit_100Gx10G-sai_impl
./bin/sai_init_and_exit_100Gx25G-sai_impl
./bin/sai_init_and_exit_100Gx50G-sai_impl
./bin/sai_init_and_exit_100Gx100G-sai_impl
./bin/sai_rib_resolution_speed-sai_impl
./bin/sai_switch_reachability_change_speed-sai_impl
```
