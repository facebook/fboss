# Agent Hw Test

## Overview

FBOSS Agent Hw tests are used to verify all FBOSS features like L3 routing, ACL, L2 learning, QoS, Copp, etc. Similar to Link Test, Agent Hw test binary is essentially a `wedge_agent` binary with Google Test (gtest) integrated.

## Setup

Unlike link tests, agent hw tests run on standalone duts and no cable connections are required. All tests involving data plane traffic are implemented by injecting packets to front panel ports put in loopback mode.

## Test Cases

All agent hw test cases could be found at

https://github.com/facebook/fboss/tree/main/fboss/agent/test/agent_hw_tests
https://github.com/facebook/fboss/tree/main/fboss/agent/hw/sai/hw_test

## Steps to Run Agent Hw Test

1. **Compile FBOSS Agent Code**
    1. Please refer to [FBOSS build documentation](https://facebook.github.io/fboss/docs/build/building_fboss_on_docker_containers/) on how to build SAI test and FBOSS wedge agent and HW test binary. `build-helper.py` will build all three binaries. [FBOSS Wiki](https://facebook.github.io/fboss/docs/build/building_fboss_on_docker_containers/).

2. **Configuration Files Used in SAI Test HW Test**
    1. OSS HW agent test configurations files used in the next section could be found under this folder `fboss/oss/hw_test_configs`. The file in general could split to two parts, FBOSS agent config and Broadcom ASIC bring up config. The FBOSS agent config is in JSON format. Broadcom ASIC bring up config is in yaml format and embeded as plain text in "platform" dicnationary in FBOSS agent JSON config. [example empty agent config with TH5 yaml ASIC config](https://github.com/facebook/fboss/blob/main/fboss/oss/hw_test_configs/montblanc.agent.materialized_JSON). This [python script](https://github.com/facebook/fboss/blob/main/fboss/lib/asic_config_v2/generate_agent_config_from_asic_config.py) could embed asic config to FBOSS agent config file.
    2. `fruid.json` determines the switch and type, and the example `fruid.json` files can be found [here](https://github.com/facebook/fboss/blob/main/fbcode/fboss/oss/scripts/run_configs/). Model name is stored in this file, please read step 5 for more details.

3. **Basic SAI Test**
    1. Here are two commands to run two basic tests, one from FBOSS agent and the other from SAI test to verify the new NPU is working.

    ```bash
    ./sai_test-sai_impl-1.16 -logging DEBUG --gtest_filter=HwVlanTest.VlanApplyConfig --flexports --fruid_filepath /home/bcmsim/config/fruid.json --config icecube_configuration/icepack.agent.materialized_JSON --logging DBG6

    ./sai_agent_hw_test --gtest_filter=AgentEmptyTest.CheckInit --fruid_filepath /home/bcmsim/config/fruid.json --config icepack.agent.materialized_JSON
    ```

    Here is an example `fruid.json` configuration file: [fruid.json](https://github.com/facebook/fboss/blob/main/fboss/lib/platforms/PlatformProductInfo.cpp).

4. **Common Debug Steps**

    ```bash
    Fboss agent calls SAI API to communicate with ASIC, here are options to collect SAI replayer.

    -enable_replayer -enable_elapsed_time_log -sai_replayer_sdk_log_level DEBUG -sai_log /root/sai_log
    ```

## Different Batches of Agent Hw Tests

When onboarding new ASIC or platforms, all eligbile hw tests not listed in below known bad test lists are supposed to pass.
https://github.com/facebook/fboss/tree/main/fboss/oss/hw_known_bad_tests

However, it is challenging to verify and debug all these hundreds of agent hw tests. Thus, they are split into three different batches of agent hw tests depending on the importance and complexity.

1. **Batch A tests**
These are simple functionality test cases that should provide basic confidence before getting the ping milestone.
- Vlan tests: all tests matching `.*Vlan.*`
- L2 learning: `.*L2ClassID.* | .*MacLearning.* | .*MacSwLearning.*`
- Neighbor resolution: `*Neighbor.*`
- L3 routing: `.*L3.* | .*HwRoute.*`
- Control Plane: `.*Copp.* | .*PacketSend.* | .*RxReason.* | .*PacketFlood.*`
- Port: `.*HwTest_PROFILE.* | .*FlextPort.*`
- Queuing: `.*SendPacketToQueue.* | .*DscpQueueMapping.* | .*PortBandwidth.*`
- Prbs: `.*PRBS.*`

2. **Batch B tests**
These are more complicated test cases that should pass, before testing teams run test plan to verify overall functionality.
- Acl: `.*Acl.* | .*DscpMarking.*`
- Warmboot: `.*warm_boot.*`
- PFC: `.*HwInPause.* | .*Pfc.*`
- QoS: `.*Qos.*`
- Aqm: `.*Aqm.*`
- ECMP: `.*Ecmp.* | .*LoadBalancer.* | .*SaiNextHopGroup.*`
- UDA ACL/Hashing: `.*HwUdfTest.*`
- Queue Per Host: `.*QueuePerHost.*`

3. **Batch C tests**
These are tests either for complicated features or related to performance tuning. Normally, they will not block FBOSS bring up on new platforms at early stages, but they will block platform qualification before deploying in production networks.
- Hashin: `.*HashPolarization.*`
- Trunk/LAG: `.*Trunk.*`
- sFlow/Mirroing: `.*Sflow.* | .*Mirror.*`
- PTP: `.*Ptp.*`
- MMU Tuning: `.*HwIngressBufferTest.* | .*MmuTuning.*`
- Stats: `.*ResourceStats.*`
