# Agent Development

## Overview

This document describes the steps involved in onboarding a new NPU to the FBOSS agent using OSS.

## Steps

1. **Build SDK and SAI SDK**
    1. Currently, FBOSS agent supports different NPU vendors like Broadcom.
    2. Broadcom SDK example:
        - [ ] FBOSS agent code needs both SAI SDK and native Broadcom SDK to function. For a new NPU, certain Broadcom SDK and SAI SDK versions are required. Please contact Broadcom for the proper release version. As mentioned in [FBOSS build documentation](https://facebook.github.io/fboss/docs/build/building_fboss_on_docker_containers/), static linked libraries from SAI SDK and Broadcom native SDK are necessary to build FBOSS agent binaries.

2. **Add New ASIC to FBOSS Agent**
    1. [Optional depending on whether ASIC is already supported]
    2. Current FBOSS supports TH, TH2, TH3, TH4, TH5, TH6 ASIC.

3. **Add New Platform Support to FBOSS Agent**
    1. The platform mapping controls how many ports are enabled, port speed, and logical port and physical port mapping. This mapping is highly dependent on hardware design. Please refer to [Platform Mapping Documentation](https://facebook.github.io/fboss/docs/developing/platform_mapping/) for more details. The platform mapping generates two files: a JSON file, which should be copied to FBOSS agent code, and a YAML file used in Broadcom SDK initialization.
    2. FBOSS agent also needs to know the platform name and the platform type to select the right platform mapping and other platform-specific files.
    3. Please refer to the "Common Code Changes" and "Agent Code Changes" in [FBOSS New Platform Support](https://facebook.github.io/fboss/docs/developing/new_platform_support/).
