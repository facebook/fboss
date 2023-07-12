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
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/PortFlowletConfig.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

namespace {

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}

const int kScalingFactor = 100;
const int kLoadWeight = 70;
const int kQueueWeight = 30;
constexpr int kDelta = 100;
} // unnamed namespace

TEST(PortFlowletConfigTest, flowletConfigName) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  registerPort(stateV0, PortID(1), "port1", scope());

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);

  config.ports()[0].flowletConfigName() = "flowletNew";

  // configured flowletConfigName but no PortFlowletConfig map exists.
  // No Throw execpetion since we don't access the option field
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);

  std::map<std::string, cfg::PortFlowletConfig> portFlowletCfgMap;
  auto updatePortFlowletCfg = [&](const std::string& key,
                                  const int scalingFactor,
                                  const int loadWeight,
                                  const int queueWeight) {
    cfg::PortFlowletConfig tmpFlowletConfig;
    tmpFlowletConfig.scalingFactor() = scalingFactor;
    tmpFlowletConfig.loadWeight() = loadWeight;
    tmpFlowletConfig.queueWeight() = queueWeight;
    portFlowletCfgMap.insert(make_pair(key, tmpFlowletConfig));
  };

  {
    cfg::PortFlowletConfig tmpFlowletConfig;
    portFlowletCfgMap.insert(make_pair("flowletOld", tmpFlowletConfig));
    config.portFlowletConfigs() = portFlowletCfgMap;

    // configured "flowletName" flowlet config  map exists.
    // but "flowletNew" doesn't exist. Only "flowletOld" does so expect an error
    EXPECT_THROW(
        publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
  }

  {
    updatePortFlowletCfg(
        "flowletNew", kScalingFactor, kLoadWeight, kQueueWeight);
    config.portFlowletConfigs() = portFlowletCfgMap;

    // update goes through now, as "flowletNew" is configured
    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);
    auto portV1 = stateV1->getPorts()->getNodeIf(PortID(1));
    ASSERT_NE(nullptr, portV1);
    EXPECT_TRUE(portV1->getFlowletConfigName().has_value());
    EXPECT_EQ(portV1->getFlowletConfigName().value(), "flowletNew");

    auto tmpFlowletCfgMap = stateV1->getPortFlowletCfgs();
    ASSERT_NE(nullptr, tmpFlowletCfgMap);
    auto portFlowletCfg = tmpFlowletCfgMap->getNodeIf("flowletNew");
    ASSERT_NE(nullptr, portFlowletCfg);
    EXPECT_EQ(portFlowletCfg->getScalingFactor(), kScalingFactor);
    EXPECT_EQ(portFlowletCfg->getLoadWeight(), kLoadWeight);
    EXPECT_EQ(portFlowletCfg->getQueueWeight(), kQueueWeight);

    EXPECT_TRUE(portV1->getPortFlowletConfig().has_value());
    auto flowletCfgPtr = portV1->getPortFlowletConfig().value();
    ASSERT_NE(nullptr, flowletCfgPtr);
    EXPECT_EQ(flowletCfgPtr->getScalingFactor(), kScalingFactor);
    EXPECT_EQ(flowletCfgPtr->getLoadWeight(), kLoadWeight);
    EXPECT_EQ(flowletCfgPtr->getQueueWeight(), kQueueWeight);

    // update the flowlet config value for  "flowletNew" and check
    portFlowletCfgMap.clear();
    updatePortFlowletCfg(
        "flowletNew", kScalingFactor + kDelta, kLoadWeight, kQueueWeight);
    config.portFlowletConfigs() = portFlowletCfgMap;
    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_NE(nullptr, stateV2);
    auto portV2 = stateV2->getPorts()->getNodeIf(PortID(1));
    ASSERT_NE(nullptr, portV2);
    EXPECT_TRUE(portV2->getFlowletConfigName().has_value());
    EXPECT_EQ(portV2->getFlowletConfigName().value(), "flowletNew");

    tmpFlowletCfgMap = stateV2->getPortFlowletCfgs();
    ASSERT_NE(nullptr, tmpFlowletCfgMap);
    portFlowletCfg = tmpFlowletCfgMap->getNodeIf("flowletNew");
    ASSERT_NE(nullptr, portFlowletCfg);
    EXPECT_EQ(portFlowletCfg->getScalingFactor(), kScalingFactor + kDelta);
    EXPECT_EQ(portFlowletCfg->getLoadWeight(), kLoadWeight);
    EXPECT_EQ(portFlowletCfg->getQueueWeight(), kQueueWeight);

    EXPECT_TRUE(portV1->getPortFlowletConfig().has_value());
    flowletCfgPtr = portV2->getPortFlowletConfig().value();
    ASSERT_NE(nullptr, flowletCfgPtr);
    EXPECT_EQ(flowletCfgPtr->getScalingFactor(), kScalingFactor + kDelta);
    EXPECT_EQ(flowletCfgPtr->getLoadWeight(), kLoadWeight);
    EXPECT_EQ(flowletCfgPtr->getQueueWeight(), kQueueWeight);
  }

  {
    // unconfigure the flowlet config
    config.ports()[0].flowletConfigName().reset();

    // port flowlet  can have any entry, shouldn't matter
    cfg::PortFlowletConfig tmpFlowletConfig;
    portFlowletCfgMap.insert(make_pair("coolFlowlet", tmpFlowletConfig));
    config.portFlowletConfigs() = portFlowletCfgMap;

    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);
    auto portV1 = stateV1->getPorts()->getNodeIf(PortID(1));
    ASSERT_NE(nullptr, portV1);
    EXPECT_FALSE(portV1->getFlowletConfigName().has_value());
  }
}

TEST(PortFlowletConfigTest, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  EXPECT_NE(nullptr, stateV0->getPortFlowletCfgs());
  EXPECT_EQ(0, stateV0->getPortFlowletCfgs()->numNodes());
  std::map<std::string, cfg::PortFlowletConfig> portFlowletCfgMap;

  auto updatePortFlowletCfg = [&](const std::string& key,
                                  const int scalingFactor,
                                  const int loadWeight,
                                  const int queueWeight) {
    cfg::PortFlowletConfig tmpFlowletConfig;
    tmpFlowletConfig.scalingFactor() = scalingFactor;
    tmpFlowletConfig.loadWeight() = loadWeight;
    tmpFlowletConfig.queueWeight() = queueWeight;
    portFlowletCfgMap.insert(make_pair(key, tmpFlowletConfig));
  };

  // add entries into the port flowlet
  updatePortFlowletCfg(
      "portFlowlet_0", kScalingFactor, kLoadWeight, kQueueWeight);
  updatePortFlowletCfg(
      "portFlowlet_1", kScalingFactor, kLoadWeight, kQueueWeight);

  config.portFlowletConfigs() = portFlowletCfgMap;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto portFlowlets = stateV1->getPortFlowletCfgs();

  EXPECT_NE(nullptr, portFlowlets);
  EXPECT_EQ(2, (*portFlowlets).numNodes());

  int index = 0;
  for (auto mMapIter : std::as_const(*portFlowlets)) {
    for (auto iter : std::as_const(*mMapIter.second)) {
      auto portFlowlet = iter.second;
      std::string portFlowletName =
          folly::to<std::string>("portFlowlet_", index);
      EXPECT_EQ(portFlowlet->getID(), portFlowletName);
      EXPECT_EQ(portFlowlet->getScalingFactor(), kScalingFactor);
      EXPECT_EQ(portFlowlet->getLoadWeight(), kLoadWeight);
      EXPECT_EQ(portFlowlet->getQueueWeight(), kQueueWeight);
      index++;
    }
  }

  {
    // push same  contents of the existing portFlowlet, ensure no change
    portFlowletCfgMap.clear();

    // add same entries to the port flowlet
    updatePortFlowletCfg(
        "portFlowlet_0", kScalingFactor, kLoadWeight, kQueueWeight);
    updatePortFlowletCfg(
        "portFlowlet_1", kScalingFactor, kLoadWeight, kQueueWeight);
    config.portFlowletConfigs() = portFlowletCfgMap;
    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_EQ(nullptr, stateV2);
  }

  {
    // modify the contents of the existing portFlowlet, bump up
    portFlowletCfgMap.clear();

    // add new entries to the port flowlet
    updatePortFlowletCfg(
        "portFlowlet_0", kScalingFactor + 10, kLoadWeight, kQueueWeight);
    updatePortFlowletCfg(
        "portFlowlet_1", kScalingFactor, kLoadWeight, kQueueWeight);
    config.portFlowletConfigs() = portFlowletCfgMap;
    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_NE(nullptr, stateV2);
  }

  {
    // add 1 more port flowlet entry, so cfg change
    // should happen, size of map changes
    updatePortFlowletCfg(
        "portFlowlet_2", kScalingFactor, kLoadWeight, kQueueWeight);
    config.portFlowletConfigs() = portFlowletCfgMap;
    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_NE(nullptr, stateV2);

    auto portFlowletCfgs = stateV2->getPortFlowletCfgs();
    EXPECT_NE(nullptr, portFlowletCfgs);
    EXPECT_EQ(portFlowletCfgMap.size(), (*portFlowletCfgs).numNodes());
  }

  config.portFlowletConfigs().reset();
  auto stateEnd = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateEnd);
  portFlowlets = stateEnd->getPortFlowletCfgs();
  EXPECT_EQ(0, portFlowlets->numNodes());
}
