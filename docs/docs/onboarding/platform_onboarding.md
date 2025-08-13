---
id: platform_onboarding
title: New Platform Onboarding
description: How to onboard a new platform to FBOSS using OSS
keywords:
    - FBOSS
    - OSS
    - platform
    - onboard
oncall: fboss_oss
---
# New Platform Onboarding

## Overview

This document describes the steps involved in onboarding a new platform to FBOSS using OSS.
For common problems and their solutions, please refer to the [troubleshooting guide](/docs/onboarding/troubleshooting/).

## Preparation Steps
1. Request access of [FBOSS OSS New Platform Onboarding Tracker Template](https://docs.google.com/spreadsheets/d/1jZiAA9cBWnQml0frf4mC5cnWqsJRL4EK7w3qiYwl5Fo/edit?usp=sharing)
2. Make a copy of this template for your new platform
3. Fill in project details in the Tracker copy and share the file to External Vendor/Meta Workplace Group
4. Regularly update the project tracker to cover new platform development/testing progress

## Steps

1. [[Manual](/docs/manuals/add_initial_support_for_platform_services/)] **Add Initial Support for Platform Services**

   **Work Items:**

   - [ ] Add platform manager support
   - [ ] Add BSP support
   - [ ] Build, run, and test platform services

   **Outcomes:**

   - [ ] `platform_manager.json` is committed to the FBOSS repository
   - [ ] `platform_manager.json` abides by the specification
   - [ ] the BSP repository is setup and accessible by Meta
   - [ ] the BSP abides by the API specification and development requirements
   - [ ] the BSP is compiled as out-of-tree kernel modules and deployed in FBOSS to unblock FBOSS development
   - [ ] `bsp_tests.json` is modified as needed based on guidance in `bsp_tests_config.thrift`
   - [ ] 100% of BSP tests pass
   - [ ] expected errors are defined in the test config if and only if the hardware has a valid reason and this is agreed to by Meta
   - [ ] platform services can be built successfully
   - [ ] platform manager can start successfully
   - [ ] 100% of platform service hardware tests pass

2. **Add Necessary Development Code**

   **Work Items:**

   - [ ] [Add Platform Mapping Config](https://facebook.github.io/fboss/docs/developing/platform_mapping/)
   - [ ] [Add BSP Mapping Config](https://facebook.github.io/fboss/docs/developing/bsp_mapping/)
   - [ ] [Add All Platform Support Code in Agent and Qsfp Service](https://facebook.github.io/fboss/docs/developing/new_platform_support/)
   - [ ] Onboard NPU [Optional depending on new platform needs]

   **Outcomes:**

   - [ ] Agent HW Test binary can be built linking to the appropriate vendor SDK
   - [ ] QSFP HW Test binary can be built
   - [ ] Link Test binary can be built linking to the appropriate vendor SDK
   - [ ] QSFP Service binary can be built
   - [ ] Wedge Agent binary can be built linking to the appropriate vendor SDK

3. **Validate Agent Change**

   **Work Items:**

   - [ ] [Build FBOSS agent test binary](https://facebook.github.io/fboss/docs/build/building_fboss_on_docker_containers/)
   - [ ] [Generate Agent HW Test Config](https://facebook.github.io/fboss/docs/build/packaging_and_running_fboss_hw_tests_on_switch/)

   **Outcomes:**

   - [ ] Agent HW Sanity Tests pass

4. **Validate Transceiver Control**

   **Work Items:**

   - [ ] [Build a QSFP/Link test setup with 100% port coverage](https://facebook.github.io/fboss/docs/testing/qsfp_and_link_test_topology/)
   - [ ] [Generate QSFP Test Config](https://facebook.github.io/fboss/docs/developing/qsfp_test_config/)

   **Outcomes:**

   - [ ] QSFP HW Sanity Tests pass

5. **Validate Link Up**

   **Work Items:**

   - [ ] Generate Link Test Config - (Currently Meta will provide a link test config)

   **Outcomes:**

   - [ ] Link Sanity Tests pass

6. **Validate Ping**

   **Work Items:**

   - [ ] [Build a Ping test setup](/docs/manuals/perform_a_ping_test/)

   **Outcomes:**

   - [ ] Ping Test passes

7. **Achieve 100% Test Pass Rate**

   **Work Items:**

   - [ ] Stabilize remaining tests

   **Outcomes:**

   - [ ] 100% QSFP HW Tests
   - [ ] 100% Link Tests
   - [ ] 100% Agent HW Tests
   - [ ] 100% Data Corral Service HW Test
   - [ ] 100% Fan Service HW Test
   - [ ] 100% Sensor Service HW Test
   - [ ] 100% Weutil HW Test
   - [ ] 100% Platform Manager HW Test
