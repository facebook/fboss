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

## Preparation Steps
1. Request access of [FBOSS OSS New Platform Onboarding Tracker Template](https://docs.google.com/spreadsheets/d/1jZiAA9cBWnQml0frf4mC5cnWqsJRL4EK7w3qiYwl5Fo/edit?usp=sharing)
2. Make a copy of this template for your new platform
3. Fill in project details in the Tracker copy and share the file to External Vendor/Meta Workplace Group
4. Regularly update the project tracker to cover new platform development/testing progress

## Steps

1. **Add Initial Support for Platform Services**

   **Work Items:**

   - [ ] [Add platform manager support](/docs/manuals/add_platform_manager_support/)
   - [ ] Add BSP support
   - [ ] Build platform services
   - [ ] Ensure platform manager can start
   - [ ] Run HW tests for platform services

   **Outcomes:**

   - [ ] Platform Services can build
   - [ ] Platform Manager can start
   - [ ] BSP can be loaded
   - [ ] Sensor Service HW Tests pass
   - [ ] Fan Service HW Tests pass
   - [ ] Platform Manager HW Tests pass

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
