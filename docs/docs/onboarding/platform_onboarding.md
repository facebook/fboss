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

## Steps

1. **Add Initial Support for Platform Services**

   **Work Items:**

   - [ ] [Add Platform Manager Support](https://facebook.github.io/fboss/docs/platform/platform_manager/)

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

   - [ ] [Build a Ping test setup](https://facebook.github.io/fboss/docs/testing/ping_test/)

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
