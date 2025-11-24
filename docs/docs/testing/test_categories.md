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

T0/T1/T2 tests are only a subset of all tests. This will help you work through
and debug issues to eventually achieve a 100% pass rate. We recommend passing
all T0 test cases first, **especially for New Platform Onboarding EVT exit**,
then T1, then T2 to make this process more organized. Then, you can run all
tests to work on achieving a 100% pass rate. This means not specifying
T0/T1/T2, a filter file, or a gtest filter so that the binary runs all tests.

Even though we recommend following this process to make testing more organized,
feel free to run all tests and work through them however you please.

## T0 Tests

### Platform Services

- all tests in `platform_hw_test`
- all tests in `data_corral_service_hw_test`
- all tests in `fan_service_hw_test`
- all tests in `fw_util_hw_test`
- all tests in `platform_manager_hw_test`
- all tests in `sensor_service_hw_test`
- all tests in `weutil_hw_test`

### Agent HW Tests

`run_test.py`:
```bash
./bin/run_test.py sai_agent \
--filter_file=./share/hw_sanity_tests/t0_agent_hw_tests.conf \
--config ./share/hw_test_configs/$CONFIG \
--enable-production-features \
--production-features ./share/production_features/asic_production_features.materialized_JSON \
--known-bad-tests-file ./share/hw_known_bad_tests/sai_agent_known_bad_tests.materialized_JSON \
--unsupported-tests-file $UNSUPPORTED_TESTS \
--asic $ASIC \ # $ASIC can be found in asic_production_features.materialized_JSON
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t0_agent_hw_tests.conf
```

### SAI Tests

`run_test.py`:
```bash
./bin/run_test.py sai \
--filter_file=./share/hw_sanity_tests/t0_sai_tests.conf \
--config ./share/hw_test_configs/$CONFIG \
--enable-production-features \
--production-features ./share/production_features/asic_production_features.materialized_JSON \
--known-bad-tests-file ./share/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON \
--unsupported-tests-file $UNSUPPORTED_TESTS \
--asic $ASIC \ # $ASIC can be found in asic_production_features.materialized_JSON
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t0_sai_tests.conf
```

### QSFP HW Tests

`run_test.py`:
```bash
./bin/run_test.py qsfp \
--filter_file=./share/hw_sanity_tests/t0_qsfp_hw_tests.conf \
--qsfp-config ./share/qsfp_test_configs/$CONFIG \
--known-bad-tests-file ./share/qsfp_known_bad_tests/fboss_qsfp_known_bad_tests.materialized_JSON \
--unsupported-tests-file ./share/qsfp_unsupported_tests/fboss_qsfp_unsupported_tests.materialized_JSON \
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t0_qsfp_hw_tests.conf
```


**Use the qsfp hw test list below for any platform that do not support Transceivers.**

`run_test.py`:
```bash
./bin/run_test.py qsfp \
--filter_file=./share/hw_sanity_tests/t0_qsfp_hw_tests_without_transceivers.conf \
--qsfp-config ./share/qsfp_test_configs/$CONFIG \
--known-bad-tests-file ./share/qsfp_known_bad_tests/fboss_qsfp_known_bad_tests.materialized_JSON \
--unsupported-tests-file ./share/qsfp_unsupported_tests/fboss_qsfp_unsupported_tests.materialized_JSON \
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t0_qsfp_hw_tests_without_transceivers.conf
```

### Link Tests

`run_test.py`:
```bash
./bin/run_test.py link \
--agent-run-mode mono \
--filter_file ./share/hw_sanity_tests/t0_ensemble_link_tests.conf \
--config ./share/link_test_configs/$CONFIG \
--qsfp-config /opt/fboss/share/qsfp_test_configs/$QSFP_CONFIG \
--known-bad-tests-file ./share/link_known_bad_tests/agent_ensemble_link_known_bad_tests.materialized_JSON \
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t0_ensemble_link_tests.conf
```


**Use the link test list below for any platform that do not support Transceivers.**

`run_test.py`:
```bash
./bin/run_test.py link \
--agent-run-mode mono \
--filter_file ./share/hw_sanity_tests/t0_ensemble_link_tests_without_transceivers.conf \
--config ./share/link_test_configs/$CONFIG \
--qsfp-config /opt/fboss/share/qsfp_test_configs/$QSFP_CONFIG \
--known-bad-tests-file ./share/link_known_bad_tests/agent_ensemble_link_known_bad_tests.materialized_JSON \
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t0_ensemble_link_tests_without_transceivers.conf
```

### BSP Tests

- All BSP tests are T0

## T1 Tests

### Agent HW Tests

`run_test.py`:
```bash
./bin/run_test.py sai_agent \
--filter_file=./share/hw_sanity_tests/t1_agent_hw_tests.conf \
--config ./share/hw_test_configs/$CONFIG \
--enable-production-features \
--production-features ./share/production_features/asic_production_features.materialized_JSON \
--known-bad-tests-file ./share/hw_known_bad_tests/sai_agent_known_bad_tests.materialized_JSON \
--unsupported-tests-file $UNSUPPORTED_TESTS \
--asic $ASIC \ # $ASIC can be found in asic_production_features.materialized_JSON
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t1_agent_hw_tests.conf
```

### Agent Benchmark Tests

- `sai_tx_slow_path_rate-sai_impl` binary
- `sai_rx_slow_path_rate-sai_impl` binary
- `sai_ecmp_shrink_speed-sai_impl` binary
- `sai_rib_resolution_speed-sai_impl` binary
- `sai_ecmp_shrink_with_competing_route_updates_speed-sai_impl` binary
- `sai_fsw_scale_route_add_speed-sai_impl` binary
- `sai_stats_collection_speed-sai_impl` binary
- `sai_init_and_exit_100Gx100G-sai_impl` binary
- `sai_switch_reachability_change_speed-sai_impl` binary

### SAI Tests

`run_test.py`:
```bash
./bin/run_test.py sai \
--filter_file=./share/hw_sanity_tests/t1_sai_tests.conf \
--config ./share/hw_test_configs/$CONFIG \
--enable-production-features \
--production-features ./share/production_features/asic_production_features.materialized_JSON \
--known-bad-tests-file ./share/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON \
--unsupported-tests-file $UNSUPPORTED_TESTS \
--asic $ASIC \ # $ASIC can be found in asic_production_features.materialized_JSON
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t1_sai_tests.conf
```

## T2 Tests

### Agent HW Tests

`run_test.py`:
```bash
./bin/run_test.py sai_agent \
--filter_file=./share/hw_sanity_tests/t2_agent_hw_tests.conf \
--config ./share/hw_test_configs/$CONFIG \
--enable-production-features \
--production-features ./share/production_features/asic_production_features.materialized_JSON \
--known-bad-tests-file ./share/hw_known_bad_tests/sai_agent_known_bad_tests.materialized_JSON \
--unsupported-tests-file $UNSUPPORTED_TESTS \
--asic $ASIC \ # $ASIC can be found in asic_production_features.materialized_JSON
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t2_agent_hw_tests.conf
```

### Agent Benchmark Tests

- `sai_fsw_scale_route_add_speed-sai_impl` binary
- `sai_hgrid_du_scale_route_add_speed-sai_impl` binary
- `sai_th_alpm_scale_route_add_speed-sai_impl` binary
- `sai_fsw_scale_route_del_speed-sai_impl` binary
- `sai_ecmp_shrink_with_competing_route_updates_speed-sai_impl` binary
- `sai_th_alpm_scale_route_del_speed-sai_impl` binary
- `sai_hgrid_du_scale_route_del_speed-sai_impl` binary
- `sai_init_and_exit_40Gx10G-sai_impl` binary
- `sai_init_and_exit_100Gx10G-sai_impl` binary
- `sai_init_and_exit_100Gx25G-sai_impl` binary
- `sai_init_and_exit_100Gx50G-sai_impl` binary
- `sai_init_and_exit_100Gx100G-sai_impl` binary
- `sai_switch_reachability_change_speed-sai_impl` binary

### SAI Tests

`run_test.py`:
```bash
./bin/run_test.py sai \
--filter_file=./share/hw_sanity_tests/t2_sai_tests.conf \
--config ./share/hw_test_configs/$CONFIG \
--enable-production-features \
--production-features ./share/production_features/asic_production_features.materialized_JSON \
--known-bad-tests-file ./share/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON \
--unsupported-tests-file $UNSUPPORTED_TESTS \
--asic $ASIC \ # $ASIC can be found in asic_production_features.materialized_JSON
--skip-known-bad-tests $KEY # $KEY can be found in known bad test file
```

```bash file=../fboss/oss/hw_sanity_tests/t2_sai_tests.conf
```
