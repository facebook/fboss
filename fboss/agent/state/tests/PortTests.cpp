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

namespace {
void prepareDefaultSwPort(Platform* platform, shared_ptr<Port> port) {
  port->setAdminState(cfg::PortState::DISABLED);
  port->setSpeed(cfg::PortSpeed::XG);
  port->setProfileId(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER);
  PlatformPortProfileConfigMatcher matcher{port->getProfileID(), port->getID()};
  if (auto profileConfig = platform->getPortProfileConfig(matcher)) {
    port->setProfileConfig(*profileConfig->iphy());
  } else {
    throw FbossError(
        "No port profile config found with matcher:", matcher.toString());
  }
  port->resetPinConfigs(
      platform->getPlatformMapping()->getPortIphyPinConfigs(matcher));
}
} // namespace

// Test to validate isAnyInterfacePortInLoopbackMode
// Validate that none of the interface/ports are in loopback mode
// modify ports to be in loopback and check again
TEST(Port, checkPortLoopbackMode) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto config = testConfigA();

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);

  for (auto iter : std::as_const(*stateV1->getInterfaces())) {
    auto intf = iter.second;
    EXPECT_FALSE(isAnyInterfacePortInLoopbackMode(stateV1, intf));
  }

  // set all ports in the configuration to loopback
  // all interfaces should also point to these loopbacks
  for (auto& port : *config.ports()) {
    port.loopbackMode() = cfg::PortLoopbackMode::PHY;
  }
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());

  for (auto iter : std::as_const(*stateV2->getInterfaces())) {
    EXPECT_TRUE(isAnyInterfacePortInLoopbackMode(stateV2, iter.second));
  }
}

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
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);
  config.ports()[0].sampleDest() = cfg::SampleDestination::MIRROR;
  *config.ports()[0].sFlowIngressRate() = 10;
  config.vlans()->resize(2);
  *config.vlans()[0].id() = 2;
  *config.vlans()[1].id() = 5;
  config.vlanPorts()->resize(2);
  *config.vlanPorts()[0].logicalPort() = 1;
  *config.vlanPorts()[0].vlanID() = 2;
  *config.vlanPorts()[0].emitTags() = false;
  *config.vlanPorts()[1].logicalPort() = 1;
  *config.vlanPorts()[1].vlanID() = 5;
  *config.vlanPorts()[1].emitTags() = true;
  config.interfaces()->resize(2);
  *config.interfaces()[0].intfID() = 2;
  *config.interfaces()[0].vlanID() = 2;
  config.interfaces()[0].mac() = "00:00:00:00:00:22";
  *config.interfaces()[1].intfID() = 5;
  *config.interfaces()[1].vlanID() = 5;
  config.interfaces()[1].mac() = "00:00:00:00:00:55";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto portV1 = stateV1->getPort(PortID(1));
  ASSERT_NE(nullptr, portV1);
  EXPECT_NE(portV0, portV1);
  EXPECT_EQ(stateV1->getPorts()->getPort("port1"), portV1);

  EXPECT_EQ(PortID(1), portV1->getID());
  EXPECT_EQ("port1", portV1->getName());
  EXPECT_EQ(1, portV1->getGeneration());
  EXPECT_EQ(cfg::PortState::ENABLED, portV1->getAdminState());
  EXPECT_EQ(cfg::PortSpeed::XG, portV1->getSpeed());
  EXPECT_EQ(
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      portV1->getProfileID());
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
  config.vlanPorts()->resize(1);
  *config.vlanPorts()[0].logicalPort() = 1;
  *config.vlanPorts()[0].vlanID() = 2021;
  *config.vlanPorts()[0].emitTags() = false;

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
  EXPECT_EQ(cfg::PortSpeed::XG, portV2->getSpeed());
  EXPECT_EQ(
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      portV2->getProfileID());
  EXPECT_FALSE(portV2->isPublished());
  EXPECT_EQ(expectedVlansV2, portV2->getVlans());
  EXPECT_TRUE(portV1->getSampleDestination().has_value());
  EXPECT_EQ(
      cfg::SampleDestination::MIRROR, portV1->getSampleDestination().value());

  // Applying the same config with a different speed should result in changes
  config.ports()[0].speed() = cfg::PortSpeed::TWENTYFIVEG;
  config.ports()[0].profileID() =
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER;

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto portV3 = stateV3->getPort(PortID(1));
  ASSERT_NE(nullptr, portV3);
  EXPECT_NE(portV2, portV3);
  EXPECT_EQ(cfg::PortSpeed::TWENTYFIVEG, portV3->getSpeed());
  EXPECT_EQ(
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      portV3->getProfileID());
}

TEST(Port, ToFromWithPgConfigs) {
  std::string jsonStr = R"(
        {
         "pgConfigs": [
            {
                "id": 0,
                "name": "first",
                "minLimitBytes": 3328,
                "scalingFactor": "ONE",
                "resumeOffsetBytes": 10,
                "bufferPoolName": ""
            },
            {
                "id": 1,
                "name": "second",
                "minLimitBytes": 4000,
                "scalingFactor": "TWO",
                "resumeOffsetBytes": 300,
                "bufferPoolName": ""
            },
            {
                "id": 2,
                "name": "third",
                "minLimitBytes": 4500,
                "scalingFactor": "FOUR",
                "resumeOffsetBytes": 400,
                "headroomLimitBytes": 2000,
                "bufferPoolName": "foo1",
                "bufferPoolConfig": {
                  "id": "foo1",
                  "headroomBytes": 2000,
                  "sharedBytes": 4000
                }
            }
          ],
          "sFlowIngressRate" : 100,
          "vlanMemberShips" : {
            "2000" : {
              "tagged" : true
            }
          },
          "queues": [
          ],
          "rxPause" : false,
          "portState" : "ENABLED",
          "portType": 0,
          "sFlowEgressRate" : 200,
          "portDescription" : "TEST",
          "portName" : "eth1/1/1",
          "portId" : 100,
          "portOperState" : true,
          "portProfileID": "PROFILE_10G_1_NRZ_NOFEC",
          "ingressVlan" : 2000,
          "portSpeed" : "XG",
          "txPause" : false,
          "sampleDest" : "MIRROR",
          "portLoopbackMode" : "PHY",
          "lookupClassesToDistrubuteTrafficOn": [
            10,
            11,
            12,
            13,
            14
          ],
          "maxFrameSize" : 9000,
          "pfc": {
            "tx": false,
            "rx": false,
            "portPgConfigName": "foo",
            "watchdog": {
              "detectionTimeMsecs": 15,
              "recoveryTimeMsecs": 16,
              "recoveryAction": 1
            }
          },
          "profileConfig": {
            "numLanes": 1,
            "modulation": 1,
            "fec": 1,
            "medium": 1,
            "interfaceMode": 10,
            "interfaceType": 10
          },
          "pinConfigs": [
            {
              "id": {
                "chip": "FC21",
                "lane": 0
              },
              "tx": {
                "pre": 2,
                "pre2": 0,
                "main": 72,
                "post": 38,
                "post2": 0,
                "post3": 0,
                "driveCurrent": 8
              }
            }
          ]
       }
  )";
  auto port = Port::fromJson(jsonStr);

  auto portPgs = port->getPortPgConfigs();
  EXPECT_TRUE(portPgs.has_value());

  auto portPgsTmp = portPgs.value();
  EXPECT_EQ(portPgsTmp.size(), 3);
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
          "portType": 0,
          "sFlowEgressRate" : 200,
          "portDescription" : "TEST",
          "portName" : "eth1/1/1",
          "portId" : 100,
          "portOperState" : true,
          "portProfileID": "PROFILE_10G_1_NRZ_NOFEC",
          "ingressVlan" : 2000,
          "portSpeed" : "XG",
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
          "maxFrameSize" : 9000,
          "pfc": {
            "tx": false,
            "rx": false,
            "portPgConfigName": "foo",
            "watchdog": {
              "detectionTimeMsecs": 15,
              "recoveryTimeMsecs": 16,
              "recoveryAction": 1
            }
          },
          "profileConfig": {
            "numLanes": 1,
            "modulation": 1,
            "fec": 1,
            "medium": 1,
            "interfaceMode": 10,
            "interfaceType": 10
          },
          "pinConfigs": [
            {
              "id": {
                "chip": "FC21",
                "lane": 0
              },
              "tx": {
                "pre": 2,
                "pre2": 0,
                "main": 72,
                "post": 38,
                "post2": 0,
                "post3": 0,
                "driveCurrent": 8
              }
            }
          ]
       }
  )";
  auto port = Port::fromJson(jsonStr);

  EXPECT_EQ(100, port->getSflowIngressRate());
  EXPECT_EQ(200, port->getSflowEgressRate());
  EXPECT_EQ(9000, port->getMaxFrameSize());
  auto vlans = port->getVlans();
  EXPECT_EQ(1, vlans.size());
  EXPECT_TRUE(vlans.at(VlanID(2000)).tagged);
  EXPECT_TRUE(*port->getPause().rx());
  EXPECT_EQ(cfg::PortState::ENABLED, port->getAdminState());
  EXPECT_EQ("TEST", port->getDescription());
  EXPECT_EQ("eth1/1/1", port->getName());
  EXPECT_EQ(PortID{100}, port->getID());
  EXPECT_EQ(PortFields::OperState::UP, port->getOperState());
  EXPECT_EQ(VlanID(2000), port->getIngressVlan());
  EXPECT_EQ(cfg::PortSpeed::XG, port->getSpeed());
  EXPECT_EQ(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC, port->getProfileID());
  EXPECT_TRUE(*port->getPause().tx());
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

  EXPECT_TRUE(port->getPfc().has_value());
  EXPECT_FALSE(*port->getPfc()->rx());
  EXPECT_FALSE(*port->getPfc()->tx());
  EXPECT_TRUE(port->getPfc()->watchdog().has_value());
  auto pfcWatchdog = port->getPfc()->watchdog().value();
  EXPECT_EQ(15, *pfcWatchdog.detectionTimeMsecs());
  EXPECT_EQ(16, *pfcWatchdog.recoveryTimeMsecs());
  EXPECT_EQ(
      cfg::PfcWatchdogRecoveryAction::DROP, *pfcWatchdog.recoveryAction());
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
          "ingressVlan" : 2000,
          "portSpeed" : "XG",
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
        (subport.portID == PortID{44}) ? cfg::LacpPortRate::SLOW
                                       : cfg::LacpPortRate::FAST,
        subport.rate);
    EXPECT_EQ(
        (subport.portID == PortID{43}) ? cfg::LacpPortActivity::PASSIVE
                                       : cfg::LacpPortActivity::ACTIVE,
        subport.activity);
    EXPECT_EQ(
        (subport.portID == PortID{42}) ? AggregatePort::Forwarding::DISABLED
                                       : AggregatePort::Forwarding::ENABLED,
        aggPort->getForwardingState(subport.portID));

    auto partnerInfo = aggPort->getPartnerState(subport.portID);
    EXPECT_EQ(
        (subport.portID == PortID{42}) ? 0
            : (subport.portID == PortID{43})
            ? (lacpState | LacpState::SHORT_TIMEOUT)
            : lacpState,
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
          "ingressVlan" : 2000,
          "portSpeed" : "XG",
          "portProfileID": "PROFILE_10G_1_NRZ_NOFEC",
          "txPause" : true
        }
  )";
  auto port = Port::fromJson(jsonStr);

  EXPECT_EQ(100, port->getSflowIngressRate());
  EXPECT_EQ(200, port->getSflowEgressRate());
  auto vlans = port->getVlans();
  EXPECT_EQ(1, vlans.size());
  EXPECT_TRUE(vlans.at(VlanID(2000)).tagged);
  EXPECT_TRUE(*port->getPause().rx());
  EXPECT_EQ(cfg::PortState::ENABLED, port->getAdminState());
  EXPECT_EQ("TEST", port->getDescription());
  EXPECT_EQ("eth1/1/1", port->getName());
  EXPECT_EQ(PortID{100}, port->getID());
  EXPECT_EQ(PortFields::OperState::UP, port->getOperState());
  EXPECT_EQ(VlanID(2000), port->getIngressVlan());
  EXPECT_EQ(cfg::PortSpeed::XG, port->getSpeed());
  EXPECT_EQ(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC, port->getProfileID());
  EXPECT_TRUE(*port->getPause().tx());
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
  auto port = state->getPorts()->getPortIf(portID);
  prepareDefaultSwPort(platform.get(), port);
  // Make sure we also update the port queues to default queue so that the
  // config change won't be triggered because of empty queue cfg.
  QueueConfig queues;
  for (int i = 0; i < platform->getAsic()->getDefaultNumPortQueues(
                          cfg::StreamType::UNICAST, false);
       i++) {
    auto queue = std::make_shared<PortQueue>(static_cast<uint8_t>(i));
    queue->setStreamType(cfg::StreamType::UNICAST);
    queues.push_back(queue);
  }
  state->getPorts()->getPortIf(portID)->resetPortQueues(queues);

  // Applying same config should result in no change.
  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(
      config.ports()[0], 1, "port1", cfg::PortState::DISABLED);
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

// validate that pause & pfc should not be enabled
// at the same time
TEST(Port, verifyPfcWithPauseConfig) {
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  state->registerPort(PortID(1), "port1");

  cfg::SwitchConfig config;
  cfg::PortPfc pfc;
  cfg::PortPause pause;

  // enable pfc, pause
  pfc.tx() = true;
  pause.rx() = true;

  config.ports()->resize(1);
  preparedMockPortConfig(
      config.ports()[0], 1, "port1", cfg::PortState::DISABLED);
  config.ports()[0].pfc() = pfc;
  config.ports()[0].pause() = pause;

  EXPECT_THROW(
      publishAndApplyConfig(state, &config, platform.get()), FbossError);
}

TEST(Port, verifyPfcConfig) {
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  state->registerPort(PortID(1), "port1");

  auto port = state->getPort(PortID(1));
  EXPECT_FALSE(port->getPfc().has_value());

  cfg::SwitchConfig config;
  cfg::PortPfc pfc;

  config.ports()->resize(1);
  preparedMockPortConfig(
      config.ports()[0], 1, "port1", cfg::PortState::DISABLED);
  config.ports()[0].pfc() = pfc;
  auto newState = publishAndApplyConfig(state, &config, platform.get());

  port = newState->getPort(PortID(1));
  EXPECT_TRUE(port->getPfc().has_value());
  EXPECT_FALSE(*port->getPfc().value().tx());
  EXPECT_FALSE(*port->getPfc().value().rx());
  EXPECT_EQ(port->getPfc().value().portPgConfigName(), "");
  EXPECT_FALSE(port->getPfc().value().watchdog().has_value());

  pfc.tx() = true;
  pfc.rx() = true;
  pfc.portPgConfigName() = "foo";

  cfg::PfcWatchdog watchdog;
  watchdog.detectionTimeMsecs() = 15;
  watchdog.recoveryTimeMsecs() = 16;
  watchdog.recoveryAction() = cfg::PfcWatchdogRecoveryAction::DROP;
  pfc.watchdog() = watchdog;

  config.ports()[0].pfc() = pfc;
  // pgConfigName exists, but can't be found in cfg, throw exception
  EXPECT_THROW(
      publishAndApplyConfig(newState, &config, platform.get()), FbossError);

  // create an empty pgConfig for "foo"
  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  portPgConfigMap["foo"] = {};
  config.portPgConfigs() = portPgConfigMap;

  auto newState2 = publishAndApplyConfig(newState, &config, platform.get());
  port = newState2->getPort(PortID(1));

  EXPECT_TRUE(port->getPfc().has_value());
  EXPECT_TRUE(*port->getPfc().value().tx());
  EXPECT_TRUE(*port->getPfc().value().rx());
  EXPECT_EQ(port->getPfc().value().portPgConfigName(), "foo");

  EXPECT_TRUE(port->getPfc()->watchdog().has_value());
  auto pfcWatchdog = port->getPfc()->watchdog().value();
  EXPECT_EQ(15, *pfcWatchdog.detectionTimeMsecs());
  EXPECT_EQ(16, *pfcWatchdog.recoveryTimeMsecs());
  EXPECT_EQ(
      cfg::PfcWatchdogRecoveryAction::DROP, *pfcWatchdog.recoveryAction());

  // Modify watchdog config and make sure it gets saved
  port = newState2->getPort(PortID(1));
  auto pfc2 = port->getPfc().value();
  cfg::PfcWatchdog watchdog2 = pfc2.watchdog().value();
  // Change the recoveryAction to NO_DROP
  watchdog2.recoveryAction() = cfg::PfcWatchdogRecoveryAction::NO_DROP;
  pfc2.watchdog() = watchdog2;
  config.ports()[0].pfc() = pfc2;

  auto newState3 = publishAndApplyConfig(newState2, &config, platform.get());
  port = newState3->getPort(PortID(1));
  EXPECT_TRUE(port->getPfc()->watchdog().has_value());
  auto pfcWatchdog2 = port->getPfc()->watchdog().value();
  EXPECT_EQ(15, *pfcWatchdog2.detectionTimeMsecs());
  EXPECT_EQ(16, *pfcWatchdog2.recoveryTimeMsecs());
  // Verify that the recoveryAction is NO_DROP as expected
  EXPECT_EQ(
      cfg::PfcWatchdogRecoveryAction::NO_DROP, *pfcWatchdog2.recoveryAction());
}

TEST(Port, pauseConfig) {
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  auto portID = PortID(1);
  state->registerPort(portID, "port1");
  auto port = state->getPorts()->getPortIf(portID);
  prepareDefaultSwPort(platform.get(), port);
  // Make sure we also update the port queues to default queue so that the
  // config change won't be triggered because of empty queue cfg
  QueueConfig queues;
  for (int i = 0; i < platform->getAsic()->getDefaultNumPortQueues(
                          cfg::StreamType::UNICAST, false);
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
    config.ports()->resize(1);
    preparedMockPortConfig(
        config.ports()[0], 1, "port1", cfg::PortState::DISABLED);
    config.ports()[0].pause() = newPause;
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

  *expected.tx() = false;
  *expected.rx() = true;
  changePause(expected);

  *expected.tx() = true;
  *expected.rx() = false;
  changePause(expected);

  *expected.tx() = true;
  *expected.rx() = true;
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
        config.ports()->resize(1);
        preparedMockPortConfig(
            config.ports()[0], 1, "port1", cfg::PortState::DISABLED);
        *config.ports()[0].loopbackMode() = newLoopbackMode;
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
        config.ports()->resize(1);
        preparedMockPortConfig(
            config.ports()[0], 1, "port1", cfg::PortState::DISABLED);
        if (newDestination.has_value()) {
          config.ports()[0].sampleDest() = newDestination.value();
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

  validateNodeMapSerialization(*ports);
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
    prepareDefaultSwPort(platform.get(), port);
    QueueConfig defaultQueues;
    for (int q = 0; q < platform->getAsic()->getDefaultNumPortQueues(
                            cfg::StreamType::UNICAST, false);
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

  validateNodeMapSerialization(*portsV0);

  EXPECT_EQ(0, portsV0->getGeneration());
  auto port1 = portsV0->getPort(PortID(1));
  auto port2 = portsV0->getPort(PortID(2));
  auto port3 = portsV0->getPort(PortID(3));
  auto port4 = portsV0->getPort(PortID(4));

  // Applying an empty config shouldn't change a newly-constructed PortMap
  cfg::SwitchConfig config;
  config.ports()->resize(4);
  for (int i = 0; i < 4; ++i) {
    preparedMockPortConfig(
        config.ports()[i],
        i + 1,
        fmt::format("port{}", i + 1),
        cfg::PortState::DISABLED);
  }
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV0, &config, platform.get()));

  // Enable port 2
  *config.ports()[1].state() = cfg::PortState::ENABLED;
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

  validateNodeMapSerialization(*portsV1);

  // Applying the same config again should do nothing.
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, platform.get()));

  // Now mark all ports up
  *config.ports()[0].state() = cfg::PortState::ENABLED;
  *config.ports()[1].state() = cfg::PortState::ENABLED;
  *config.ports()[2].state() = cfg::PortState::ENABLED;
  *config.ports()[3].state() = cfg::PortState::ENABLED;

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
  preparedMockPortConfig(config.ports()[0], 1);
  preparedMockPortConfig(config.ports()[1], 2);
  preparedMockPortConfig(
      config.ports()[2], 3, "port3", cfg::PortState::DISABLED);
  preparedMockPortConfig(config.ports()[3], 4);
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
  validateNodeMapSerialization(*portsV3);
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

  validateNodeMapSerialization(*ports);
}

TEST(Port, portFabricType) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto config = testConfigA();

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);

  for (auto port : *stateV1->getPorts()) {
    EXPECT_EQ(port->getPortType(), cfg::PortType::INTERFACE_PORT);
    auto newPort = port->clone();
    newPort->setPortType(cfg::PortType::FABRIC_PORT);
    auto newerPort = Port::fromFollyDynamic(newPort->toFollyDynamic());
    EXPECT_EQ(newerPort->getPortType(), cfg::PortType::FABRIC_PORT);
  }
}

TEST(Port, portFabricTypeApplyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto config = testConfigA();

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);

  for (auto& port : *config.ports()) {
    EXPECT_EQ(port.portType(), cfg::PortType::INTERFACE_PORT);
    port.portType() = cfg::PortType::FABRIC_PORT;
  }
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  for (auto port : *stateV2->getPorts()) {
    EXPECT_EQ(port->getPortType(), cfg::PortType::FABRIC_PORT);
    EXPECT_NE(*port, *stateV1->getPorts()->getPort(port->getID()));
  }
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV2, &config, platform.get()));
  // Flip back to intefac_port type
  for (auto& port : *config.ports()) {
    port.portType() = cfg::PortType::INTERFACE_PORT;
  }
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  for (auto port : *stateV3->getPorts()) {
    EXPECT_EQ(port->getPortType(), cfg::PortType::INTERFACE_PORT);
    EXPECT_NE(*port, *stateV2->getPorts()->getPort(port->getID()));
  }
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV3, &config, platform.get()));
}

TEST(Port, portSerilization) {
  auto port = std::make_shared<Port>(PortID(42), "test_port");
  EXPECT_EQ(port->getName(), "test_port");
  EXPECT_EQ(port->getID(), PortID(42));

  // IphyLinkStatus
  EXPECT_EQ(port->getIPhyLinkFaultStatus(), std::nullopt);
  phy::LinkFaultStatus linkFaultStatus;
  linkFaultStatus.localFault() = true;
  port->setIPhyLinkFaultStatus(linkFaultStatus);
  EXPECT_TRUE(port->getIPhyLinkFaultStatus().has_value());
  EXPECT_EQ(port->getIPhyLinkFaultStatus().value(), linkFaultStatus);

  // AsicPrbs, GbSystemPrbs & GbLinePrbs
  phy::PortPrbsState prbsState;
  EXPECT_EQ(port->getAsicPrbs(), prbsState);
  EXPECT_EQ(port->getGbSystemPrbs(), prbsState);
  EXPECT_EQ(port->getGbLinePrbs(), prbsState);
  prbsState.polynominal() = 42;
  port->setAsicPrbs(prbsState);
  port->setGbSystemPrbs(prbsState);
  port->setGbLinePrbs(prbsState);
  EXPECT_EQ(port->getAsicPrbs(), prbsState);
  EXPECT_EQ(port->getGbSystemPrbs(), prbsState);
  EXPECT_EQ(port->getGbLinePrbs(), prbsState);

  // Pfc Priorities
  EXPECT_EQ(port->getPfcPriorities(), std::nullopt);
  std::optional<std::vector<PfcPriority>> pfcPriorities;
  pfcPriorities.emplace({PfcPriority(42)});
  port->setPfcPriorities(pfcPriorities);
  EXPECT_TRUE(port->getPfcPriorities().has_value());
  EXPECT_EQ(port->getPfcPriorities().value().size(), 1);

  // expected LLDP values
  EXPECT_TRUE(port->getLLDPValidations().empty());
  PortFields::LLDPValidations lldpMap{{cfg::LLDPTag::TTL, "ttlTag"}};
  port->setExpectedLLDPValues(lldpMap);
  EXPECT_EQ(port->getLLDPValidations().size(), 1);

  // RxSaks
  EXPECT_TRUE(port->getRxSaks().empty());
  PortFields::RxSaks rxSaks{
      {PortFields::MKASakKey{mka::MKASci{}, 42}, mka::MKASak{}}};
  port->setRxSaks(rxSaks);
  EXPECT_EQ(port->getRxSaks().size(), 1);

  // txSecureAssociationKey
  EXPECT_EQ(port->getTxSak(), std::nullopt);
  port->setTxSak(mka::MKASak{});
  EXPECT_TRUE(port->getTxSak().has_value());

  EXPECT_FALSE(port->getMacsecDesired());
  EXPECT_FALSE(port->getDropUnencrypted());
  port->setMacsecDesired(true);
  port->setDropUnencrypted(true);
  EXPECT_TRUE(port->getMacsecDesired());
  EXPECT_TRUE(port->getDropUnencrypted());

  EXPECT_EQ(*port, *Port::fromThrift(port->toThrift()));
}
