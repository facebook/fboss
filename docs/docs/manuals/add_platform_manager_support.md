---
id: add_platform_manager_support
title: Add Platform Manager Support
description: How to add platform manager support
keywords:
    - FBOSS
    - OSS
    - platform
    - onboard
    - manual
oncall: fboss_oss
---

## Overview

PlatformManager support is required for all other platform services to run. Read more about PlatformManager [here](/docs/platform/platform_manager/).

## Make Necessary Code Changes
Please refer to the [Vendor Expectations](/docs/platform/platform_manager/#vendor-expectations) section
of the platform manager document for more detail.

### Platform Configuration
You need to create the platform specific configuration according to the
[specification](https://github.com/facebook/fboss/tree/main/fboss/platform/configs/sample) located in the FBOSS repository.

### BSP Support
You need to ensure the BSP follows the [requirements specification](/docs/platform/bsp_api_specification/) to enable
device discovery and setup. There are also [development requirements](/docs/platform/bsp_development_requirements/)
that must be followed.

## Build Platform Services
In this step, we will use `getdeps.py` to build the platform services and hardware test binaries. Detailed instructions
for building FBOSS are contained in this [guide](/docs/build/building_fboss_on_docker_containers/). Read and follow along,
using instructions for building using fake SAI when appropriate.

**Note:** Refer to [this section](/docs/build/building_fboss_on_docker_containers/#limit-the-build-to-a-specific-target)
of the build guide to speed up your build. Use `--cmake-target fboss_platform_services`.

## Outcomes
- the configuration abides by the specification
- the configuration is committed to the FBOSS repository
- the BSP abides by the specification and development requirements
- the BSP is compiled as out-of-tree kernel modules and deployed in FBOSS to unblock FBOSS development
- platform services are built via `getdeps.py`
