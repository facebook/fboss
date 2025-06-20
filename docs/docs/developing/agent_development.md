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

4. **Compile FBOSS Agent Code**
    1. Please refer to [FBOSS build documentation](https://facebook.github.io/fboss/docs/build/building_fboss_on_docker_containers/) on how to build SAI test and FBOSS wedge agent and HW test binary. `build-helper.py` will build all three binaries. [FBOSS Wiki](https://facebook.github.io/fboss/docs/build/building_fboss_on_docker_containers/).

5. **Configuration Files Used in SAI Test HW Test**
    1. OSS HW agent test used in the next section could be found under this folder `fboss/oss/hw_test_configs`.
    2. `fruid.json` determines the switch and type, and the example `fruid.json` files can be found [here](https://github.com/facebook/fboss/blob/main/fbcode/fboss/oss/scripts/run_configs/). Model name is stored in this file, please read step 5 for more details.

6. **Basic SAI Test**
    1. Here are two commands to run two basic tests, one from FBOSS agent and the other from SAI test to verify the new NPU is working.

    ```bash
    ./sai_test-sai_impl-1.16 -logging DEBUG --gtest_filter=HwVlanTest.VlanApplyConfig --flexports --fruid_filepath /home/bcmsim/config/fruid.json --config icecube_configuration/icepack.agent.materialized_JSON --logging DBG6

    ./sai_agent_hw_test --gtest_filter=AgentEmptyTest.CheckInit --fruid_filepath /home/bcmsim/config/fruid.json --config icepack.agent.materialized_JSON
    ```

    Here is an example `fruid.json` configuration file: [fruid.json](https://github.com/facebook/fboss/blob/main/fboss/lib/platforms/PlatformProductInfo.cpp).

7. **Common Debug Steps**

    ```bash
    Fboss agent calls SAI API to communicate with ASIC, here are options to collect SAI replayer.

    -enable_replayer -enable_elapsed_time_log -sai_replayer_sdk_log_level DEBUG -sai_log /root/sai_log
    ```
