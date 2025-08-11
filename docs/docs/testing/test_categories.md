---
id: test_categories
title: T0/T1/T2 Tests
description: Tests listed by priority and category
keywords:
    - FBOSS
    - OSS
    - onboard
    - test
    - T0
    - T1
    - T2
oncall: fboss_oss
---

## Overview

We have many tests to verify platforms and it can be overwhelming to know which
ones to prioritize. This page outlines different tests categorized by type and
priority.

The different priorities are as follows:
- T0 - Test cases that check simple yet critical functionality and are very
important to enable other tests to pass.
- T1 - Test cases that are a little more complicated and will help verify
overall functionality.
- T2 - Test cases that are either for complicated features or related to
performance tuning. Normally, they will not block FBOSS bring up on new
platforms at early stages, but they will block platform qualification before
deploying in production networks.

## T0 Tests

### Platform Services

- all tests in `data_corral_service_hw_test`
- all tests in `fan_service_hw_test`
- all tests in `fw_util_hw_test`
- all tests in `platform_manager_hw_test`
- all tests in `sensor_service_hw_test`
- all tests in `weutil_hw_test`

### Agent HW Tests

`run_test.py`:
```bash
./bin/run_test.py sai_agent --filter_file=./share/hw_sanity_tests/t0_agent_hw_tests.conf --config ./share/hw_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t0_agent_hw_tests.conf
```

### SAI Tests

`run_test.py`:
```bash
./bin/run_test.py sai --filter_file=./share/hw_sanity_tests/t0_sai_tests.conf --config ./share/hw_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t0_sai_tests.conf
```

### QSFP HW Tests

`run_test.py`:
```bash
./bin/run_test.py qsfp --filter_file=./share/hw_sanity_tests/t0_qsfp_hw_tests.conf --qsfp-config ./share/qsfp_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t0_qsfp_hw_tests.conf
```

### Link Tests

`run_test.py`:
```bash
./bin/run_test.py link --filter_file=./share/hw_sanity_tests/t0_link_tests.conf --config ./share/link_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t0_link_tests.conf
```

### BSP Tests

- All BSP tests are T0

## T1 Tests

### Agent HW Tests

`run_test.py`:
```bash
./bin/run_test.py sai_agent --filter_file=./share/hw_sanity_tests/t1_agent_hw_tests.conf --config ./share/hw_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t1_agent_hw_tests.conf
```

### SAI Tests

`run_test.py`:
```bash
./bin/run_test.py sai --filter_file=./share/hw_sanity_tests/t1_sai_tests.conf --config ./share/hw_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t1_sai_tests.conf
```

## T2 Tests

### Agent HW Tests

`run_test.py`:
```bash
./bin/run_test.py sai_agent --filter_file=./share/hw_sanity_tests/t2_agent_hw_tests.conf --config ./share/hw_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t2_agent_hw_tests.conf
```

### SAI Tests

`run_test.py`:
```bash
./bin/run_test.py sai --filter_file=./share/hw_sanity_tests/t2_sai_tests.conf --config ./share/hw_test_configs/$CONFIG ...
```

```bash file=../fboss/oss/hw_sanity_tests/t2_sai_tests.conf
```
