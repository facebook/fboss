/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

TEST(Port, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto portV0 = stateV0->getPort(PortID(1));
  EXPECT_EQ(0, portV0->getGeneration());
  EXPECT_FALSE(portV0->isPublished());
  EXPECT_EQ(PortID(1), portV0->getID());
  EXPECT_EQ("port1", portV0->getName());
  EXPECT_EQ(cfg::PortState::DISABLED, portV0->getAdminState());
  Port::VlanMembership emptyVlans;
  EXPECT_EQ(emptyVlans, portV0->getVlans());
  EXPECT_FALSE(portV0->getSampleDestination().has_value());

  portV0->publish();
  EXPECT_TRUE(portV0->isPublished());

  cfg::SwitchConfig config;
  config.ports_ref()->resize(1);
  *config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  *config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;
  config.ports_ref()[0].sampleDest_ref() = cfg::SampleDestination::MIRROR;
  *config.ports_ref()[0].sFlowIngressRate_ref() = 10;
  config.vlans_ref()->resize(2);
  *config.vlans_ref()[0].id_ref() = 2;
  *config.vlans_ref()[1].id_ref() = 5;
  config.vlanPorts_ref()->resize(2);
  *config.vlanPorts_ref()[0].logicalPort_ref() = 1;
  *config.vlanPorts_ref()[0].vlanID_ref() = 2;
  *config.vlanPorts_ref()[0].emitTags_ref() = false;
  *config.vlanPorts_ref()[1].logicalPort_ref() = 1;
  *config.vlanPorts_ref()[1].vlanID_ref() = 5;
  *config.vlanPorts_ref()[1].emitTags_ref() = true;
  config.interfaces_ref()->resize(2);
  *config.interfaces_ref()[0].intfID_ref() = 2;
  *config.interfaces_ref()[0].vlanID_ref() = 2;
  config.interfaces_ref()[0].mac_ref() = "00:00:00:00:00:22";
  *config.interfaces_ref()[1].intfID_ref() = 5;
  *config.interfaces_ref()[1].vlanID_ref() = 5;
  config.interfaces_ref()[1].mac_ref() = "00:00:00:00:00:55";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto portV1 = stateV1->getPort(PortID(1));
  ASSERT_NE(nullptr, portV1);
  EXPECT_NE(portV0, portV1);

  EXPECT_EQ(PortID(1), portV1->getID());
  EXPECT_EQ("port1", portV1->getName());
  EXPECT_EQ(1, portV1->getGeneration());
  EXPECT_EQ(cfg::PortState::ENABLED, portV1->getAdminState());
  EXPECT_FALSE(portV1->isPublished());
  Port::VlanMembership expectedVlans;
  expectedVlans.insert(make_pair(VlanID(2), Port::VlanInfo(false)));
  expectedVlans.insert(make_pair(VlanID(5), Port::VlanInfo(true)));
  EXPECT_EQ(expectedVlans, portV1->getVlans());
  EXPECT_TRUE(portV1->getSampleDestination().has_value());
  EXPECT_EQ(
      cfg::SampleDestination::MIRROR, portV1->getSampleDestination().value());

  // Applying the same config again should result in no changes
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, platform.get()));

  // Applying the same config with a new VLAN list should result in changes
  config.vlanPorts_ref()->resize(1);
  *config.vlanPorts_ref()[0].logicalPort_ref() = 1;
  *config.vlanPorts_ref()[0].vlanID_ref() = 2021;
  *config.vlanPorts_ref()[0].emitTags_ref() = false;

  Port::VlanMembership expectedVlansV2;
  expectedVlansV2.insert(make_pair(VlanID(2021), Port::VlanInfo(false)));
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  auto portV2 = stateV2->getPort(PortID(1));
  ASSERT_NE(nullptr, portV2);
  EXPECT_NE(portV1, portV2);

  EXPECT_EQ(PortID(1), portV2->getID());
  EXPECT_EQ("port1", portV2->getName());
  EXPECT_EQ(2, portV2->getGeneration());
  EXPECT_EQ(cfg::PortState::ENABLED, portV2->getAdminState());
  EXPECT_FALSE(portV2->isPublished());
  EXPECT_EQ(expectedVlansV2, portV2->getVlans());
  EXPECT_TRUE(portV1->getSampleDestination().has_value());
  EXPECT_EQ(
      cfg::SampleDestination::MIRROR, portV1->getSampleDestination().value());

  // Applying the same config with a different speed should result in changes
  *config.ports_ref()[0].speed_ref() = cfg::PortSpeed::GIGE;

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto portV3 = stateV3->getPort(PortID(1));
  ASSERT_NE(nullptr, portV3);
  EXPECT_NE(portV2, portV3);
  EXPECT_EQ(cfg::PortSpeed::GIGE, portV3->getSpeed());
}

TEST(Port, ToFromJSON) {
  std::string jsonStr = R"(
        {
          "queues" : [
            {
                "streamType": "UNICAST",
                "weight": 1,
                "reserved": 3328,
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 0,
                "scalingFactor": "ONE"
            },
            {
                "streamType": "UNICAST",
                "weight": 9,
                "reserved": 19968,
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 1,
                "scalingFactor": "EIGHT"
            },
            {
                "streamType": "UNICAST",
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 2,
                "weight": 1
            },
            {
                "streamType": "UNICAST",
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 3,
                "weight": 1
            }
          ],
          "sFlowIngressRate" : 100,
          "vlanMemberShips" : {
            "2000" : {
              "tagged" : true
            }
          },
          "rxPause" : true,
          "portState" : "ENABLED",
          "sFlowEgressRate" : 200,
          "portDescription" : "TEST",
          "portName" : "eth1/1/1",
          "portId" : 100,
          "portOperState" : true,
          "portProfileID": "PROFILE_10G_1_NRZ_NOFEC",
          "portMaxSpeed" : "XG",
          "ingressVlan" : 2000,
          "portSpeed" : "XG",
          "portFEC" : "OFF",
          "txPause" : true,
          "sampleDest" : "MIRROR",
          "portLoopbackMode" : "PHY",
          "lookupClassesToDistrubuteTrafficOn": [
            10,
            11,
            12,
            13,
            14
          ],
          "maxFrameSize" : 9000
        }
  )";
  auto port = Port::fromJson(jsonStr);

  EXPECT_EQ(100, port->getSflowIngressRate());
  EXPECT_EQ(200, port->getSflowEgressRate());
  EXPECT_EQ(9000, port->getMaxFrameSize());
  auto vlans = port->getVlans();
  EXPECT_EQ(1, vlans.size());
  EXPECT_TRUE(vlans.at(VlanID(2000)).tagged);
  EXPECT_TRUE(*port->getPause().rx_ref());
  EXPECT_EQ(cfg::PortState::ENABLED, port->getAdminState());
  EXPECT_EQ("TEST", port->getDescription());
  EXPECT_EQ("eth1/1/1", port->getName());
  EXPECT_EQ(PortID{100}, port->getID());
  EXPECT_EQ(PortFields::OperState::UP, port->getOperState());
  EXPECT_EQ(VlanID(2000), port->getIngressVlan());
  EXPECT_EQ(cfg::PortSpeed::XG, port->getSpeed());
  EXPECT_EQ(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC, port->getProfileID());
  EXPECT_EQ(cfg::PortFEC::OFF, port->getFEC());
  EXPECT_TRUE(*port->getPause().tx_ref());
  EXPECT_EQ(cfg::PortLoopbackMode::PHY, port->getLoopbackMode());
  EXPECT_TRUE(port->getSampleDestination().has_value());
  EXPECT_EQ(
      cfg::SampleDestination::MIRROR, port->getSampleDestination().value());

  auto queues = port->getPortQueues();
  EXPECT_EQ(4, queues.size());

  EXPECT_EQ(cfg::StreamType::UNICAST, queues[0]->getStreamType());
  EXPECT_EQ(3328, queues[0]->getReservedBytes().value());
  EXPECT_EQ(1, queues[0]->getWeight());
  EXPECT_EQ(0, queues[0]->getID());
  EXPECT_EQ(
      cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN, queues[0]->getScheduling());
  EXPECT_EQ(cfg::MMUScalingFactor::ONE, queues[0]->getScalingFactor().value());

  EXPECT_EQ(cfg::StreamType::UNICAST, queues[1]->getStreamType());
  EXPECT_EQ(19968, queues[1]->getReservedBytes().value());
  EXPECT_EQ(9, queues[1]->getWeight());
  EXPECT_EQ(1, queues[1]->getID());
  EXPECT_EQ(
      cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN, queues[1]->getScheduling());
  EXPECT_EQ(
      cfg::MMUScalingFactor::EIGHT, queues[1]->getScalingFactor().value());

  EXPECT_EQ(cfg::StreamType::UNICAST, queues[2]->getStreamType());
  EXPECT_FALSE(queues[2]->getReservedBytes().has_value());
  EXPECT_EQ(1, queues[2]->getWeight());
  EXPECT_EQ(2, queues[2]->getID());
  EXPECT_EQ(
      cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN, queues[2]->getScheduling());
  EXPECT_FALSE(queues[2]->getScalingFactor().has_value());

  EXPECT_EQ(cfg::StreamType::UNICAST, queues[3]->getStreamType());
  EXPECT_FALSE(queues[3]->getReservedBytes().has_value());
  EXPECT_EQ(1, queues[3]->getWeight());
  EXPECT_EQ(3, queues[3]->getID());
  EXPECT_EQ(
      cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN, queues[3]->getScheduling());
  EXPECT_FALSE(queues[3]->getScalingFactor().has_value());

  std::vector<cfg::AclLookupClass> expectedLookupClasses = {
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4};
  EXPECT_EQ(
      port->getLookupClassesToDistributeTrafficOn(), expectedLookupClasses);

  auto dyn1 = port->toFollyDynamic();
  auto dyn2 = folly::parseJson(jsonStr);

  EXPECT_EQ(dyn1, dyn2);
}

TEST(Port, ToFromJSONMissingMaxFrameSize) {
  std::string jsonStr = R"(
        {
          "queues" : [
            {
                "streamType": "UNICAST",
                "weight": 1,
                "reserved": 3328,
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 0,
                "scalingFactor": "ONE"
            },
            {
                "streamType": "UNICAST",
                "weight": 9,
                "reserved": 19968,
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 1,
                "scalingFactor": "EIGHT"
            },
            {
                "streamType": "UNICAST",
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 2,
                "weight": 1
            },
            {
                "streamType": "UNICAST",
                "scheduling": "WEIGHTED_ROUND_ROBIN",
                "id": 3,
                "weight": 1
            }
          ],
          "sFlowIngressRate" : 100,
          "vlanMemberShips" : {
            "2000" : {
              "tagged" : true
            }
          },
          "rxPause" : true,
          "portState" : "ENABLED",
          "sFlowEgressRate" : 200,
          "portDescription" : "TEST",
          "portName" : "eth1/1/1",
          "portId" : 100,
          "portOperState" : true,
          "portProfileID": "PROFILE_10G_1_NRZ_NOFEC",
          "portMaxSpeed" : "XG",
          "ingressVlan" : 2000,
          "portSpeed" : "XG",
          "portFEC" : "OFF",
          "txPause" : true,
          "sampleDest" : "MIRROR",
          "portLoopbackMode" : "PHY"
        }
  )";
  auto port = Port::fromJson(jsonStr);

  EXPECT_EQ(
      cfg::switch_config_constants::DEFAULT_PORT_MTU(),
      port->getMaxFrameSize());
}

TEST(AggregatePort, ToFromJSON) {
  std::string jsonStr = R"(
        {
          "id": 10,
          "name": "tr0",
          "description": "Some trunk port",
          "systemPriority": 10,
          "systemID": "12:42:00:22:53:01",
          "minimumLinkCount": 2,
          "subports": [
            {
              "portId": 42,
              "priority": 1,
              "rate": "fast",
              "activity": "active",
              "holdTimerMultiplier": 3
            },
            {
              "portId": 43,
              "priority": 1,
              "rate": "fast",
              "activity": "passive",
              "holdTimerMultiplier": 3
            },
            {
              "portId": 44,
              "priority": 1,
              "rate": "slow",
              "activity": "active",
              "holdTimerMultiplier": 3
            }
          ],
          "forwardingStates": [
            {
              "portId": 42,
              "forwarding": "disabled"
            },
            {
              "portId": 43,
              "forwarding": "enabled"
            },
            {
              "portId": 44,
              "forwarding": "enabled"
            }
          ],
          "partnerInfos": [
            {
              "portId": 42,
              "partnerInfo": {
                "id": 52,
                "portId": 152,
                "priority": 1,
                "state": 0,
                "systemID": [ 12, 42, 00, 32, 10, 01],
                "systemPriority": 10
              }
            },
            {
              "portId": 43,
              "partnerInfo": {
                "id": 53,
                "portId": 153,
                "priority": 1,
                "state": 63,
                "systemID": [ 12, 42, 00, 32, 10, 02],
                "systemPriority": 10
              }
            },
            {
              "portId": 44,
              "partnerInfo": {
                "id": 54,
                "portId": 154,
                "priority": 1,
                "state": 61,
                "systemID": [ 12, 42, 00, 32, 10, 03],
                "systemPriority": 10
              }
            }
          ]
        }
  )";
  auto aggPort = AggregatePort::fromJson(jsonStr);

  EXPECT_EQ(AggregatePortID(10), aggPort->getID());
  EXPECT_EQ("tr0", aggPort->getName());
  EXPECT_EQ("Some trunk port", aggPort->getDescription());
  EXPECT_EQ(10, aggPort->getSystemPriority());
  EXPECT_EQ(folly::MacAddress("12:42:00:22:53:01"), aggPort->getSystemID());
  EXPECT_EQ(2, aggPort->getMinimumLinkCount());
  EXPECT_EQ(3, aggPort->subportsCount());

  auto lacpState = LacpState::ACTIVE | LacpState::AGGREGATABLE |
      LacpState::COLLECTING | LacpState::DISTRIBUTING | LacpState::IN_SYNC;

  for (const auto& subport : aggPort->sortedSubports()) {
    EXPECT_EQ(1, subport.priority);
    EXPECT_EQ(
        subport.portID == 44 ? cfg::LacpPortRate::SLOW
                             : cfg::LacpPortRate::FAST,
        subport.rate);
    EXPECT_EQ(
        subport.portID == 43 ? cfg::LacpPortActivity::PASSIVE
                             : cfg::LacpPortActivity::ACTIVE,
        subport.activity);
    EXPECT_EQ(
        subport.portID == 42 ? AggregatePort::Forwarding::DISABLED
                             : AggregatePort::Forwarding::ENABLED,
        aggPort->getForwardingState(subport.portID));

    auto partnerInfo = aggPort->getPartnerState(subport.portID);
    EXPECT_EQ(
        subport.portID == 42
            ? 0
            : (subport.portID == 43 ? lacpState | LacpState::SHORT_TIMEOUT
                                    : lacpState),
        partnerInfo.state);
  }

  auto dyn1 = aggPort->toFollyDynamic();
  auto dyn2 = folly::parseJson(jsonStr);

  EXPECT_EQ(dyn1, dyn2);
}

TEST(Port, ToFromJSONLoopbackModeMissingFromJson) {
  std::string jsonStr = R"(
        {
          "queues" : [
          ],
          "sFlowIngressRate" : 100,
          "vlanMemberShips" : {
            "2000" : {
              "tagged" : true
            }
          },
          "rxPause" : true,
          "portState" : "ENABLED",
          "sFlowEgressRate" : 200,
          "portDescription" : "TEST",
          "portName" : "eth1/1/1",
          "portId" : 100,
          "portOperState" : true,
          "portMaxSpeed" : "XG",
          "ingressVlan" : 2000,
          "portSpeed" : "XG",
          "portProfileID": "PROFILE_10G_1_NRZ_NOFEC",
          "portFEC" : "OFF",
          "txPause" : true
        }
  )";
  auto port = Port::fromJson(jsonStr);

  EXPECT_EQ(100, port->getSflowIngressRate());
  EXPECT_EQ(200, port->getSflowEgressRate());
  auto vlans = port->getVlans();
  EXPECT_EQ(1, vlans.size());
  EXPECT_TRUE(vlans.at(VlanID(2000)).tagged);
  EXPECT_TRUE(*port->getPause().rx_ref());
  EXPECT_EQ(cfg::PortState::ENABLED, port->getAdminState());
  EXPECT_EQ("TEST", port->getDescription());
  EXPECT_EQ("eth1/1/1", port->getName());
  EXPECT_EQ(PortID{100}, port->getID());
  EXPECT_EQ(PortFields::OperState::UP, port->getOperState());
  EXPECT_EQ(VlanID(2000), port->getIngressVlan());
  EXPECT_EQ(cfg::PortSpeed::XG, port->getSpeed());
  EXPECT_EQ(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC, port->getProfileID());
  EXPECT_EQ(cfg::PortFEC::OFF, port->getFEC());
  EXPECT_TRUE(*port->getPause().tx_ref());
  EXPECT_EQ(cfg::PortLoopbackMode::NONE, port->getLoopbackMode());

  auto queues = port->getPortQueues();
  EXPECT_EQ(0, queues.size());

  auto dyn1 = port->toFollyDynamic();
  auto dyn2 = Port::fromJson(jsonStr)->toFollyDynamic();

  EXPECT_EQ(dyn1, dyn2);
}

TEST(Port, emptyConfig) {
  auto platform = createMockPlatform();
  PortID portID(1);
  auto state = make_shared<SwitchState>();
  state->registerPort(portID, "port1");
  state->getPorts()->getPortIf(portID)->setAdminState(cfg::PortState::DISABLED);
  // Make sure we also update the port queues to default queue so that the
  // config change won't be triggered because of empty queue cfg.
  QueueConfig queues;
  for (int i = 0; i <
       platform->getAsic()->getDefaultNumPortQueues(cfg::StreamType::UNICAST);
       i++) {
    auto queue = std::make_shared<PortQueue>(static_cast<uint8_t>(i));
    queue->setStreamType(cfg::StreamType::UNICAST);
    queues.push_back(queue);
  }
  state->getPorts()->getPortIf(portID)->resetPortQueues(queues);

  // Applying same config should result in no change.
  cfg::SwitchConfig config;
  config.ports_ref()->resize(1);
  config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  config.ports_ref()[0].state_ref() = cfg::PortState::DISABLED;
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, platform.get()));

  // If platform does not support addRemovePort (by default),
  // empty config should throw exception.
  cfg::SwitchConfig emptyConfig;
  EXPECT_THROW(
      publishAndApplyConfig(state, &emptyConfig, platform.get()), FbossError);

  // If platform supports addRemovePort, change should happen.
  ON_CALL(*platform.get(), supportsAddRemovePort())
      .WillByDefault(testing::Return(true));
  EXPECT_NE(
      nullptr, publishAndApplyConfig(state, &emptyConfig, platform.get()));
}

TEST(Port, pauseConfig) {
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  state->registerPort(PortID(1), "port1");
  // Make sure we also update the port queues to default queue so that the
  // config change won't be triggered because of empty queue cfg
  QueueConfig queues;
  for (int i = 0; i <
       platform->getAsic()->getDefaultNumPortQueues(cfg::StreamType::UNICAST);
       i++) {
    auto queue = std::make_shared<PortQueue>(static_cast<uint8_t>(i));
    queue->setStreamType(cfg::StreamType::UNICAST);
    queues.push_back(queue);
  }
  state->getPorts()->getPortIf(PortID(1))->resetPortQueues(queues);

  auto verifyPause = [&state](cfg::PortPause expectPause) {
    auto port = state->getPort(PortID(1));
    auto pause = port->getPause();
    EXPECT_EQ(expectPause, pause);
  };

  auto changePause = [&](cfg::PortPause newPause) {
    auto oldPause = state->getPort(PortID(1))->getPause();
    cfg::SwitchConfig config;
    config.ports_ref()->resize(1);
    *config.ports_ref()[0].logicalID_ref() = 1;
    config.ports_ref()[0].name_ref() = "port1";
    *config.ports_ref()[0].state_ref() = cfg::PortState::DISABLED;
    *config.ports_ref()[0].pause_ref() = newPause;
    auto newState = publishAndApplyConfig(state, &config, platform.get());

    if (oldPause != newPause) {
      EXPECT_NE(nullptr, newState);
      state = newState;
      verifyPause(newPause);
    } else {
      EXPECT_EQ(nullptr, newState);
    }
  };

  // Verify the default pause config is no pause for either tx or rx
  cfg::PortPause expected;
  verifyPause(expected);

  // Now change it each time
  changePause(expected);

  *expected.tx_ref() = false;
  *expected.rx_ref() = true;
  changePause(expected);

  *expected.tx_ref() = true;
  *expected.rx_ref() = false;
  changePause(expected);

  *expected.tx_ref() = true;
  *expected.rx_ref() = true;
  changePause(expected);
}

TEST(Port, loopbackModeConfig) {
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  state->registerPort(PortID(1), "port1");
  auto verifyLoopbackMode = [&state](cfg::PortLoopbackMode expectLoopbackMode) {
    auto port = state->getPort(PortID(1));
    auto loopbackMode = port->getLoopbackMode();
    EXPECT_EQ(expectLoopbackMode, loopbackMode);
  };

  auto changeAndVerifyLoopbackMode =
      [&](cfg::PortLoopbackMode newLoopbackMode) {
        auto oldLoopbackMode = state->getPort(PortID(1))->getLoopbackMode();
        cfg::SwitchConfig config;
        config.ports_ref()->resize(1);
        *config.ports_ref()[0].logicalID_ref() = 1;
        config.ports_ref()[0].name_ref() = "port1";
        *config.ports_ref()[0].state_ref() = cfg::PortState::DISABLED;
        *config.ports_ref()[0].loopbackMode_ref() = newLoopbackMode;
        auto newState = publishAndApplyConfig(state, &config, platform.get());

        if (oldLoopbackMode != newLoopbackMode) {
          EXPECT_NE(nullptr, newState);
          state = newState;
          verifyLoopbackMode(newLoopbackMode);
        } else {
          EXPECT_EQ(nullptr, newState);
        }
      };

  // Verify the default loopback mode is NONE
  cfg::PortLoopbackMode expected{cfg::PortLoopbackMode::NONE};
  verifyLoopbackMode(expected);

  // Now change it each time

  expected = cfg::PortLoopbackMode::PHY;
  changeAndVerifyLoopbackMode(expected);

  expected = cfg::PortLoopbackMode::MAC;
  changeAndVerifyLoopbackMode(expected);

  expected = cfg::PortLoopbackMode::NONE;
  changeAndVerifyLoopbackMode(expected);
}

TEST(Port, sampleDestinationConfig) {
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  state->registerPort(PortID(1), "port1");
  auto changeAndVerifySampleDestination =
      [](std::unique_ptr<MockPlatform>& platform,
         std::shared_ptr<SwitchState>& state,
         std::optional<cfg::SampleDestination> newDestination) {
        auto oldDestination = state->getPort(PortID(1))->getSampleDestination();
        cfg::SwitchConfig config;
        config.ports_ref()->resize(1);
        *config.ports_ref()[0].logicalID_ref() = 1;
        config.ports_ref()[0].name_ref() = "port1";
        *config.ports_ref()[0].state_ref() = cfg::PortState::DISABLED;
        if (newDestination.has_value()) {
          config.ports_ref()[0].sampleDest_ref() = newDestination.value();
        }
        auto newState = publishAndApplyConfig(state, &config, platform.get());

        if (oldDestination != newDestination) {
          EXPECT_NE(nullptr, newState);
          state = newState;
          auto portSampleDest =
              state->getPort(PortID(1))->getSampleDestination();
          EXPECT_EQ(portSampleDest, newDestination);
        } else {
          EXPECT_EQ(nullptr, newState);
        }
      };

  // Verify the default sample destination is NONE
  EXPECT_EQ(std::nullopt, state->getPort(PortID(1))->getSampleDestination());

  // Now change it and verify change is properly configured
  changeAndVerifySampleDestination(
      platform, state, cfg::SampleDestination::MIRROR);
  changeAndVerifySampleDestination(
      platform, state, cfg::SampleDestination::CPU);
  changeAndVerifySampleDestination(platform, state, std::nullopt);
}

TEST(PortMap, registerPorts) {
  auto ports = make_shared<PortMap>();
  EXPECT_EQ(0, ports->getGeneration());
  EXPECT_FALSE(ports->isPublished());
  EXPECT_EQ(0, ports->numPorts());

  ports->registerPort(PortID(1), "port1");
  ports->registerPort(PortID(2), "port2");
  ports->registerPort(PortID(3), "port3");
  ports->registerPort(PortID(4), "port4");
  EXPECT_EQ(4, ports->numPorts());

  auto port1 = ports->getPort(PortID(1));
  auto port2 = ports->getPort(PortID(2));
  auto port3 = ports->getPort(PortID(3));
  auto port4 = ports->getPort(PortID(4));
  EXPECT_EQ(PortID(1), port1->getID());
  EXPECT_EQ("port1", port1->getName());
  EXPECT_EQ(PortID(4), port4->getID());
  EXPECT_EQ("port4", port4->getName());

  // Attempting to register a duplicate port ID should fail
  EXPECT_THROW(ports->registerPort(PortID(2), "anotherPort2"), FbossError);

  // Registering non-sequential IDs should work
  ports->registerPort(PortID(10), "port10");
  EXPECT_EQ(5, ports->numPorts());
  auto port10 = ports->getPort(PortID(10));
  EXPECT_EQ(PortID(10), port10->getID());
  EXPECT_EQ("port10", port10->getName());

  // Getting non-existent ports should fail
  EXPECT_THROW(ports->getPort(PortID(0)), FbossError);
  EXPECT_THROW(ports->getPort(PortID(7)), FbossError);
  EXPECT_THROW(ports->getPort(PortID(300)), FbossError);

  // Publishing the PortMap should also mark all ports as published
  ports->publish();
  EXPECT_TRUE(ports->isPublished());
  EXPECT_TRUE(port1->isPublished());
  EXPECT_TRUE(port2->isPublished());
  EXPECT_TRUE(port3->isPublished());
  EXPECT_TRUE(port4->isPublished());
  EXPECT_TRUE(port10->isPublished());

  // Attempting to register new ports after the PortMap has been published
  // should crash.
  ASSERT_DEATH(
      ports->registerPort(PortID(5), "port5"), "Check failed: !isPublished()");
}

/*
 * Test that forEachChanged(StateDelta::getPortsDelta(), ...) invokes the
 * callback for the specified list of changed ports.
 */
void checkChangedPorts(
    const shared_ptr<PortMap>& oldPorts,
    const shared_ptr<PortMap>& newPorts,
    const std::set<uint16_t> changedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetPorts(oldPorts);
  auto newState = make_shared<SwitchState>();
  newState->resetPorts(newPorts);

  std::set<uint16_t> invokedPorts;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        EXPECT_EQ(oldPort->getID(), newPort->getID());
        EXPECT_NE(oldPort, newPort);

        auto ret = invokedPorts.insert(oldPort->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, invokedPorts);
}

TEST(PortMap, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto portsV0 = stateV0->getPorts();
  auto registerPort = [&](int i) {
    auto port =
        std::make_shared<Port>(PortID(i), folly::format("port{}", i).str());
    QueueConfig defaultQueues;
    for (int q = 0; q <
         platform->getAsic()->getDefaultNumPortQueues(cfg::StreamType::UNICAST);
         q++) {
      auto defaultQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(q));
      defaultQueue->setStreamType(cfg::StreamType::UNICAST);
      defaultQueues.push_back(defaultQueue);
    }
    port->resetPortQueues(defaultQueues);
    portsV0->addPort(port);
  };
  for (int i = 1; i <= 4; i++) {
    registerPort(i);
  }
  portsV0->publish();

  EXPECT_EQ(0, portsV0->getGeneration());
  auto port1 = portsV0->getPort(PortID(1));
  auto port2 = portsV0->getPort(PortID(2));
  auto port3 = portsV0->getPort(PortID(3));
  auto port4 = portsV0->getPort(PortID(4));

  // Applying an empty config shouldn't change a newly-constructed PortMap
  cfg::SwitchConfig config;
  config.ports_ref()->resize(4);
  *config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  *config.ports_ref()[1].logicalID_ref() = 2;
  config.ports_ref()[1].name_ref() = "port2";
  *config.ports_ref()[2].logicalID_ref() = 3;
  config.ports_ref()[2].name_ref() = "port3";
  *config.ports_ref()[3].logicalID_ref() = 4;
  config.ports_ref()[3].name_ref() = "port4";
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV0, &config, platform.get()));

  // Enable port 2
  *config.ports_ref()[1].state_ref() = cfg::PortState::ENABLED;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto portsV1 = stateV1->getPorts();
  ASSERT_NE(nullptr, portsV1);
  EXPECT_EQ(1, portsV1->getGeneration());
  EXPECT_EQ(4, portsV1->numPorts());

  // Only port 2 should have changed
  EXPECT_EQ(port1, portsV1->getPort(PortID(1)));
  EXPECT_NE(port2, portsV1->getPort(PortID(2)));
  EXPECT_EQ(port3, portsV1->getPort(PortID(3)));
  EXPECT_EQ(port4, portsV1->getPort(PortID(4)));
  checkChangedPorts(portsV0, portsV1, {2});

  auto newPort2 = portsV1->getPort(PortID(2));
  EXPECT_EQ(cfg::PortState::ENABLED, newPort2->getAdminState());
  EXPECT_EQ(cfg::PortState::DISABLED, port1->getAdminState());
  EXPECT_EQ(cfg::PortState::DISABLED, port3->getAdminState());
  EXPECT_EQ(cfg::PortState::DISABLED, port4->getAdminState());

  // The new PortMap and port 2 should still be unpublished.
  // The remaining other ports are the same and were previously published
  EXPECT_FALSE(portsV1->isPublished());
  EXPECT_FALSE(newPort2->isPublished());
  EXPECT_TRUE(port1->isPublished());
  // Publish portsV1 now.
  portsV1->publish();
  EXPECT_TRUE(portsV1->isPublished());
  EXPECT_TRUE(newPort2->isPublished());
  EXPECT_TRUE(port1->isPublished());

  // Applying the same config again should do nothing.
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, platform.get()));

  // Now mark all ports up
  *config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;
  *config.ports_ref()[1].state_ref() = cfg::PortState::ENABLED;
  *config.ports_ref()[2].state_ref() = cfg::PortState::ENABLED;
  *config.ports_ref()[3].state_ref() = cfg::PortState::ENABLED;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  auto portsV2 = stateV2->getPorts();
  ASSERT_NE(nullptr, portsV2);
  EXPECT_EQ(2, portsV2->getGeneration());

  EXPECT_NE(port1, portsV2->getPort(PortID(1)));
  EXPECT_EQ(newPort2, portsV2->getPort(PortID(2)));
  EXPECT_NE(port3, portsV2->getPort(PortID(3)));
  EXPECT_NE(port4, portsV2->getPort(PortID(4)));

  EXPECT_EQ(
      cfg::PortState::ENABLED, portsV2->getPort(PortID(1))->getAdminState());
  EXPECT_EQ(
      cfg::PortState::ENABLED, portsV2->getPort(PortID(2))->getAdminState());
  EXPECT_EQ(
      cfg::PortState::ENABLED, portsV2->getPort(PortID(3))->getAdminState());
  EXPECT_EQ(
      cfg::PortState::ENABLED, portsV2->getPort(PortID(4))->getAdminState());
  checkChangedPorts(portsV1, portsV2, {1, 3, 4});

  EXPECT_FALSE(portsV2->getPort(PortID(1))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(2))->isPublished());
  EXPECT_FALSE(portsV2->getPort(PortID(3))->isPublished());
  EXPECT_FALSE(portsV2->getPort(PortID(4))->isPublished());
  portsV2->publish();
  EXPECT_TRUE(portsV2->getPort(PortID(1))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(2))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(3))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(4))->isPublished());

  // If we disable port3 from the config, it should be marked down
  config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;
  config.ports_ref()[1].logicalID_ref() = 2;
  config.ports_ref()[1].name_ref() = "port2";
  config.ports_ref()[1].state_ref() = cfg::PortState::ENABLED;
  config.ports_ref()[2].logicalID_ref() = 3;
  config.ports_ref()[2].name_ref() = "port3";
  config.ports_ref()[2].state_ref() = cfg::PortState::DISABLED;
  config.ports_ref()[3].logicalID_ref() = 4;
  config.ports_ref()[3].name_ref() = "port4";
  config.ports_ref()[3].state_ref() = cfg::PortState::ENABLED;
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto portsV3 = stateV3->getPorts();
  ASSERT_NE(nullptr, portsV3);
  EXPECT_EQ(3, portsV3->getGeneration());

  EXPECT_EQ(4, portsV3->numPorts());
  EXPECT_EQ(
      cfg::PortState::ENABLED, portsV3->getPort(PortID(1))->getAdminState());
  EXPECT_EQ(
      cfg::PortState::ENABLED, portsV3->getPort(PortID(2))->getAdminState());
  EXPECT_EQ(
      cfg::PortState::DISABLED, portsV3->getPort(PortID(3))->getAdminState());
  EXPECT_EQ(
      cfg::PortState::ENABLED, portsV3->getPort(PortID(4))->getAdminState());
  checkChangedPorts(portsV2, portsV3, {3});
}

TEST(PortMap, iterateOrder) {
  // The NodeMapDelta::Iterator code assumes that the PortMap iterator walks
  // through the ports in sorted order (sorted by PortID).
  //
  // Add a test to ensure that this always remains true.  (If we ever change
  // the underlying map data structure used for PortMap, we will need to update
  // the StateDelta code.)
  auto ports = make_shared<PortMap>();
  ports->registerPort(PortID(99), "a");
  ports->registerPort(PortID(37), "b");
  ports->registerPort(PortID(88), "c");
  ports->registerPort(PortID(4), "d");
  ports->publish();

  auto it = ports->begin();
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(4), (*it)->getID());
  EXPECT_EQ("d", (*it)->getName());

  ++it;
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(37), (*it)->getID());
  EXPECT_EQ("b", (*it)->getName());

  ++it;
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(88), (*it)->getID());
  EXPECT_EQ("c", (*it)->getName());

  ++it;
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(99), (*it)->getID());
  EXPECT_EQ("a", (*it)->getName());

  ++it;
  EXPECT_EQ(ports->end(), it);
}
