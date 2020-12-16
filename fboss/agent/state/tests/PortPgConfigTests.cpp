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
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

namespace {
constexpr int kStateTestNumPortPgs = 4;
} // unnamed namespace

TEST(PortPgConfig, TestPortPgNameMismatch) {
  int pgId = 0;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.ports_ref()->resize(1);
  config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;

  cfg::PortPfc pfc;
  pfc.portPgConfigName_ref() = "not_foo";
  config.ports_ref()[0].pfc_ref() = pfc;

  // Case 1
  // port points to pgConfig named "not_foo", but it doesn't exist in cfg
  // throw an error
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;
  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id_ref() = pgId;
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap["foo"] = portPgConfigs;
  // add pgConfig with name "foo"
  config.portPgConfigs_ref() = portPgConfigMap;

  // Case 2
  // port points to "not_foo", but pg config exists only for "foo"
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);

  // undo the changes in the port pfc struct, now all is cleaned, back to case 1
  config.portPgConfigs_ref().reset();
  // since cleanup of pfc name on port hasn't happened,
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(PortPgConfig, TestPortPgMaxLimit) {
  int pgId = 0;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.ports_ref()->resize(1);
  config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;

  // pg_id exceeded more than PG_MAX
  for (pgId = 0; pgId < cfg::switch_config_constants::PORT_PG_MAX() + 1;
       pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id_ref() = pgId;
    pgConfig.name_ref() = folly::to<std::string>("pg", pgId);
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap["foo"] = portPgConfigs;

  cfg::PortPfc pfc;
  pfc.portPgConfigName_ref() = "foo";

  // assign pfc, pg cfgs
  config.ports_ref()[0].pfc_ref() = pfc;
  config.portPgConfigs_ref() = portPgConfigMap;

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(PortPgConfig, ApplyConfig) {
  int pgId = 0;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.ports_ref()->resize(1);
  config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;
  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id_ref() = pgId;
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap["foo"] = portPgConfigs;

  cfg::PortPfc pfc;
  pfc.portPgConfigName_ref() = "foo";
  config.ports_ref()[0].pfc_ref() = pfc;
  config.portPgConfigs_ref() = portPgConfigMap;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto pgCfgs = stateV1->getPort(PortID(1))->getPortPgConfigs();
  EXPECT_TRUE(pgCfgs);

  auto pgCfgValue = pgCfgs.value();
  EXPECT_EQ(kStateTestNumPortPgs, pgCfgValue.size());

  auto createPortPgConfig = [&](const int pgIdStart, const int pgIdEnd) {
    portPgConfigs.clear();
    for (pgId = pgIdStart; pgId < pgIdEnd; pgId++) {
      cfg::PortPgConfig pgConfig;
      pgConfig.id_ref() = pgId;
      pgConfig.name_ref() = folly::to<std::string>("pg", pgId);
      pgConfig.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
      pgConfig.minLimitBytes_ref() = 1000;
      pgConfig.resumeOffsetBytes_ref() = 100;
      pgConfig.bufferPoolName_ref() = "bufferName";
      portPgConfigs.emplace_back(pgConfig);
    }
    portPgConfigMap["foo"] = portPgConfigs;
    config.portPgConfigs_ref() = portPgConfigMap;
  };

  // validate that for same number of PGs, if contents differ
  // we push the change
  createPortPgConfig(
      0 /* pg start index */, kStateTestNumPortPgs /* pg end index */);
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);

  auto pgCfgs1 = stateV2->getPort(PortID(1))->getPortPgConfigs();
  EXPECT_TRUE(pgCfgs1);
  pgCfgValue = pgCfgs1.value();
  EXPECT_EQ(kStateTestNumPortPgs, pgCfgValue.size());

  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    auto name = folly::to<std::string>("pg", pgId);
    EXPECT_EQ(pgCfgValue[pgId]->getName(), name);
    EXPECT_EQ(
        pgCfgValue[pgId]->getScalingFactor(), cfg::MMUScalingFactor::EIGHT);
    EXPECT_EQ(pgCfgValue[pgId]->getMinLimitBytes(), 1000);
    EXPECT_EQ(pgCfgValue[pgId]->getBufferPoolName(), "bufferName");
  }

  {
    // validate that for the same PG content, but different PG size between old
    // and new we push the change we are changing pg size from
    // kStateTestNumPortPgs -> PORT_PG_MAX()
    createPortPgConfig(0, cfg::switch_config_constants::PORT_PG_MAX());
    auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
    EXPECT_NE(nullptr, stateV3);

    auto pgCfgsNew = stateV3->getPort(PortID(1))->getPortPgConfigs();
    EXPECT_TRUE(pgCfgsNew);
    pgCfgValue = pgCfgsNew.value();
    EXPECT_EQ(cfg::switch_config_constants::PORT_PG_MAX(), pgCfgValue.size());
  }

  {
    // validate that for the same content, same PG size but different PG ID
    // between old and new we push the change
    // we are changing the pg id from
    // {0, kStateTestNumPortPgs} -> {1, kStateTestNumPortPgs+1}
    createPortPgConfig(1, kStateTestNumPortPgs + 1);
    auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
    EXPECT_NE(nullptr, stateV3);

    auto pgCfgsNew = stateV3->getPort(PortID(1))->getPortPgConfigs();
    EXPECT_TRUE(pgCfgsNew);
    pgCfgValue = pgCfgsNew.value();
    EXPECT_EQ(kStateTestNumPortPgs, pgCfgValue.size());

    // no cfg change, ensure that PG logic can detect no change
    auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
    EXPECT_EQ(nullptr, stateV4);
  }

  // undo the changes in the port pfc struct, now all is cleaned
  config.portPgConfigs_ref().reset();
  config.ports_ref()[0].pfc_ref().reset();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());

  EXPECT_NE(nullptr, stateV3);
  EXPECT_FALSE(stateV3->getPort(PortID(1))->getPortPgConfigs());
}
