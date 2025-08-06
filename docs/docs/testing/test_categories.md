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

- Vlan tests: all tests matching `.*Vlan.*`
- L2 learning: `.*L2ClassID.* | .*MacLearning.* | .*MacSwLearning.*`
- Neighbor resolution: `*Neighbor.*`
- L3 routing: `.*L3.* | .*HwRoute.*`
- Control Plane: `.*Copp.* | .*PacketSend.* | .*RxReason.* | .*PacketFlood.*`
- Port: `.*HwTest_PROFILE.* | .*FlextPort.*`
- Queuing: `.*SendPacketToQueue.* | .*DscpQueueMapping.* | .*PortBandwidth.*`
- Prbs: `.*PRBS.*`

### QSFP HW Tests

- EmptyHwTest.CheckInit
- HwTest.i2cStressRead
- HwTest.i2cStressWrite
- HwTransceiverResetTest.verifyResetControl
- HwTransceiverResetTest.resetTranscieverAndDetectPresence
- HwStateMachineTest.CheckPortsProgrammed
- HwTest.cmisPageChange

### Link Tests

- AgentEnsembleEmptyLinkTest.CheckInit
- AgentEnsembleLinkTest.trafficRxTx
- AgentEnsembleLinkTest.asicLinkFlap
- AgentEnsembleLinkTest.getTransceivers
- AgentEnsembleLinkTest.iPhyInfoTest

## T1 Tests

### Agent HW Tests

- Acl: `.*Acl.* | .*DscpMarking.*`
- Warmboot: `.*warm_boot.*`
- PFC: `.*HwInPause.* | .*Pfc.*`
- QoS: `.*Qos.*`
- Aqm: `.*Aqm.*`
- ECMP: `.*Ecmp.* | .*LoadBalancer.* | .*SaiNextHopGroup.*`
- UDA ACL/Hashing: `.*HwUdfTest.*`
- Queue Per Host: `.*QueuePerHost.*`

## T2 Tests

### Agent HW Tests

- Hashing: `.*HashPolarization.*`
- Trunk/LAG: `.*Trunk.*`
- sFlow/Mirroing: `.*Sflow.* | .*Mirror.*`
- PTP: `.*Ptp.*`
- MMU Tuning: `.*HwIngressBufferTest.* | .*MmuTuning.*`
- Stats: `.*ResourceStats.*`
