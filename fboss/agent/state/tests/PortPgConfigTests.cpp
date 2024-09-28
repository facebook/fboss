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
#include "fboss/agent/EnumUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Port.h"
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
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);

  cfg::PortPfc pfc;
  pfc.portPgConfigName() = "not_foo";
  config.ports()[0].pfc() = pfc;

  // Case 1
  // port points to pgConfig named "not_foo", but it doesn't exist in cfg
  // throw an error
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;
  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap["foo"] = portPgConfigs;
  // add pgConfig with name "foo"
  config.portPgConfigs() = portPgConfigMap;

  // Case 2
  // port points to "not_foo", but pg config exists only for "foo"
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);

  // undo the changes in the port pfc struct, now all is cleaned, back to case 1
  config.portPgConfigs().reset();
  // since cleanup of pfc name on port hasn't happened,
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(PortPgConfig, TestPortPgMaxLimit) {
  int pgId = 0;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  config.ports()[0].logicalID() = 1;
  config.ports()[0].name() = "port1";
  config.ports()[0].state() = cfg::PortState::ENABLED;

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;

  // pg_id should be [0, PORT_PG_VALUE_MAX]
  // Lets exceed more than PG_MAX
  for (pgId = 0; pgId <= cfg::switch_config_constants::PORT_PG_VALUE_MAX() + 1;
       pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    pgConfig.name() = folly::to<std::string>("pg", pgId);
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap["foo"] = portPgConfigs;

  cfg::PortPfc pfc;
  pfc.portPgConfigName() = "foo";

  // assign pfc, pg cfgs
  config.ports()[0].pfc() = pfc;
  config.portPgConfigs() = portPgConfigMap;

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(PortPgConfig, TestPortPgWatchdogConfigMismatch) {
  // Test to verify that priority group is associated
  // to a port when PFC watchdog is configured.
  // Test creates a port config and assocites PFC watchdog
  // but leaves the priority group name empty
  // and expects exception to be thrown.
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);

  cfg::PortPfc pfc;
  cfg::PfcWatchdog watchdog;
  watchdog.detectionTimeMsecs() = 15;
  watchdog.recoveryTimeMsecs() = 16;
  watchdog.recoveryAction() = cfg::PfcWatchdogRecoveryAction::NO_DROP;
  pfc.watchdog() = watchdog;
  pfc.portPgConfigName() = "";
  config.ports()[0].pfc() = pfc;

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);

  // Make sure there is no exception when priority group is populated
  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;
  for (int pgId = 0; pgId < 1; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap["foo"] = portPgConfigs;
  // add pgConfig with name "foo"
  config.portPgConfigs() = portPgConfigMap;
  pfc.portPgConfigName() = "foo";
  config.ports()[0].pfc() = pfc;

  EXPECT_NO_THROW(publishAndApplyConfig(stateV0, &config, platform.get()));
}

TEST(PortPgConfig, applyConfig) {
  int pgId = 0;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  constexpr folly::StringPiece kBufferPoolName = "bufferPool";
  constexpr folly::StringPiece kPgConfigName = "foo";

  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
  cfg::SwitchConfig config;
  config.ports()->resize(2);
  preparedMockPortConfig(config.ports()[0], 1);
  preparedMockPortConfig(config.ports()[1], 2);

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;
  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap[kPgConfigName.str()] = portPgConfigs;

  cfg::PortPfc pfc;
  pfc.portPgConfigName() = kPgConfigName.str();
  config.ports()[0].pfc() = pfc;
  config.portPgConfigs() = portPgConfigMap;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto pgCfgs = stateV1->getPort(PortID(1))->getPortPgConfigs();
  EXPECT_TRUE(pgCfgs);
  EXPECT_EQ(kStateTestNumPortPgs, pgCfgs->size());

  auto createPortPgConfig = [&](const int pgIdStart,
                                const int pgIdEnd,
                                const std::string& pgConfigName,
                                const std::string& bufferName) {
    portPgConfigs.clear();
    for (pgId = pgIdStart; pgId < pgIdEnd; pgId++) {
      cfg::PortPgConfig pgConfig;
      pgConfig.id() = pgId;
      pgConfig.name() = folly::to<std::string>("pg", pgId);
      pgConfig.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
      pgConfig.minLimitBytes() = 1000;
      pgConfig.resumeOffsetBytes() = 100;
      pgConfig.bufferPoolName() = bufferName;
      portPgConfigs.emplace_back(pgConfig);
    }
    portPgConfigMap[pgConfigName] = portPgConfigs;
    config.portPgConfigs() = portPgConfigMap;
  };

  auto createBufferPoolCfg = [&](const int sharedBytes,
                                 const int headroomBytes,
                                 const std::string& bufferName) {
    cfg::BufferPoolConfig tmpPoolConfig1;
    tmpPoolConfig1.headroomBytes() = headroomBytes;
    tmpPoolConfig1.sharedBytes() = sharedBytes;
    bufferPoolCfgMap.insert(make_pair(bufferName, tmpPoolConfig1));
    config.bufferPoolConfigs() = bufferPoolCfgMap;
  };

  // validate that for same number of PGs, if contents differ
  // we push the change
  createPortPgConfig(
      0 /* pg start index */,
      kStateTestNumPortPgs /* pg end index */,
      kPgConfigName.str(),
      kBufferPoolName.str());
  cfg::BufferPoolConfig tmpPoolConfig;
  bufferPoolCfgMap.insert(make_pair(kBufferPoolName.str(), tmpPoolConfig));
  config.bufferPoolConfigs() = bufferPoolCfgMap;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);

  auto pgCfgs1 = stateV2->getPort(PortID(1))->getPortPgConfigs();
  EXPECT_TRUE(pgCfgs1);
  EXPECT_EQ(kStateTestNumPortPgs, pgCfgs1->size());

  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    auto name = folly::to<std::string>("pg", pgId);
    EXPECT_EQ(pgCfgs1->at(pgId)->cref<switch_state_tags::name>()->cref(), name);
    cfg::MMUScalingFactor scalingFactor = nameToEnum<cfg::MMUScalingFactor>(
        pgCfgs1->at(pgId)->cref<switch_state_tags::scalingFactor>()->cref());
    EXPECT_EQ(scalingFactor, cfg::MMUScalingFactor::EIGHT);
    EXPECT_EQ(
        pgCfgs1->at(pgId)->cref<switch_state_tags::minLimitBytes>()->cref(),
        1000);
    EXPECT_EQ(
        pgCfgs1->at(pgId)->cref<switch_state_tags::bufferPoolName>()->cref(),
        kBufferPoolName.str());
  }

  {
    // validate that for the same PG content, but different PG size between old
    // and new we push the change we are changing pg size from
    // kStateTestNumPortPgs -> PORT_PG_VALUE_MAX()
    createPortPgConfig(
        0,
        cfg::switch_config_constants::PORT_PG_VALUE_MAX(),
        kPgConfigName.str(),
        kBufferPoolName.str());
    auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
    EXPECT_NE(nullptr, stateV3);

    auto pgCfgsNew = stateV3->getPort(PortID(1))->getPortPgConfigs();
    EXPECT_TRUE(pgCfgsNew);
    EXPECT_EQ(
        cfg::switch_config_constants::PORT_PG_VALUE_MAX(), pgCfgsNew->size());
  }

  {
    // validate that for the same content, same PG size but different PG ID
    // between old and new we push the change
    // we are changing the pg id from
    // {0, kStateTestNumPortPgs} -> {1, kStateTestNumPortPgs+1}
    createPortPgConfig(
        1,
        kStateTestNumPortPgs + 1,
        kPgConfigName.str(),
        kBufferPoolName.str());
    auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
    EXPECT_NE(nullptr, stateV3);

    auto pgCfgsNew = stateV3->getPort(PortID(1))->getPortPgConfigs();
    EXPECT_TRUE(pgCfgsNew);
    EXPECT_EQ(kStateTestNumPortPgs, pgCfgsNew->size());

    // no cfg change, ensure that PG logic can detect no change
    auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
    EXPECT_EQ(nullptr, stateV4);
  }

  {
    constexpr int kBufferHdrmBytes = 2000;
    constexpr int kBufferSharedBytes = 3000;
    constexpr int kDelta = 1;
    // validate that new bufferPol cfg results in new updates
    // appropriately reflected in the PortPg config
    createPortPgConfig(0, 1, kPgConfigName.str(), kBufferPoolName.str());
    bufferPoolCfgMap.clear();
    createBufferPoolCfg(
        kBufferSharedBytes, kBufferHdrmBytes, kBufferPoolName.str());
    auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
    EXPECT_NE(nullptr, stateV3);

    const auto& pgCfgsNew = stateV3->getPort(PortID(1))->getPortPgConfigs();

    // TODO(zecheng): Make type inference work with lambda
    EXPECT_TRUE(pgCfgsNew);
    for (const auto& pgCfg : std::as_const(*pgCfgsNew)) {
      const auto bufferPoolCfg =
          pgCfg->cref<switch_state_tags::bufferPoolConfig>();
      EXPECT_EQ(
          bufferPoolCfg->cref<switch_state_tags::id>()->cref(),
          kBufferPoolName.str());
      EXPECT_EQ(
          bufferPoolCfg->cref<common_if_tags::sharedBytes>()->cref(),
          kBufferSharedBytes);
      EXPECT_EQ(
          bufferPoolCfg->cref<common_if_tags::headroomBytes>()->cref(),
          kBufferHdrmBytes);
    }

    // modify contents of the buffer pool
    // ensure they get reflected
    bufferPoolCfgMap.clear();
    createBufferPoolCfg(
        kBufferSharedBytes + kDelta,
        kBufferHdrmBytes + kDelta,
        kBufferPoolName.str());
    auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
    EXPECT_NE(nullptr, stateV4);
    const auto& pgCfgsNew1 = stateV4->getPort(PortID(1))->getPortPgConfigs();

    EXPECT_TRUE(pgCfgsNew1);
    for (const auto& pgCfg : std::as_const(*pgCfgsNew1)) {
      const auto bufferPoolCfg =
          pgCfg->cref<switch_state_tags::bufferPoolConfig>();
      EXPECT_EQ(
          bufferPoolCfg->cref<switch_state_tags::id>()->cref(),
          kBufferPoolName.str());
      EXPECT_EQ(
          bufferPoolCfg->cref<common_if_tags::sharedBytes>()->cref(),
          kBufferSharedBytes + kDelta);
      EXPECT_EQ(
          bufferPoolCfg->cref<common_if_tags::headroomBytes>()->cref(),
          kBufferHdrmBytes + kDelta);
    }

    // no change expected here
    bufferPoolCfgMap.clear();
    createBufferPoolCfg(
        kBufferSharedBytes + kDelta,
        kBufferHdrmBytes + kDelta,
        kBufferPoolName.str());
    auto stateV5 = publishAndApplyConfig(stateV4, &config, platform.get());
    EXPECT_EQ(nullptr, stateV5);

    // reset the bufferPool, ensure thats cleaned up from the PgConfig
    bufferPoolCfgMap.clear();
    createPortPgConfig(0, 1, kPgConfigName.str(), "");
    config.bufferPoolConfigs() = bufferPoolCfgMap;
    stateV5 = publishAndApplyConfig(stateV4, &config, platform.get());
    EXPECT_NE(nullptr, stateV5);
    const auto& pgCfgsNew2 = stateV5->getPort(PortID(1))->getPortPgConfigs();
    for (const auto& pgCfg : *pgCfgsNew2) {
      EXPECT_FALSE(pgCfg->cref<switch_state_tags::bufferPoolConfig>());
    }
  }

  // undo the changes in the port pfc struct, now all is cleaned
  config.portPgConfigs().reset();
  config.ports()[0].pfc().reset();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());

  EXPECT_NE(nullptr, stateV3);
  EXPECT_FALSE(stateV3->getPort(PortID(1))->getPortPgConfigs());

  {
    // validate bufferpool name is same across all ports.
    auto stateV6 = make_shared<SwitchState>();

    createPortPgConfig(
        0 /* pg start index */,
        kStateTestNumPortPgs /* pg end index */,
        "pgMapCfg1",
        "bufferPool1");

    createPortPgConfig(
        0 /* pg start index */,
        kStateTestNumPortPgs /* pg end index */,
        "pgMapCfg2",
        "bufferPool2");

    constexpr int kBufferHdrmBytes = 2000;
    constexpr int kBufferSharedBytes = 3000;

    bufferPoolCfgMap.clear();
    createBufferPoolCfg(kBufferSharedBytes, kBufferHdrmBytes, "bufferPool1");
    createBufferPoolCfg(kBufferSharedBytes, kBufferHdrmBytes, "bufferPool2");

    pfc.portPgConfigName() = "pgMapCfg1";
    config.ports()[0].pfc() = pfc;

    pfc.portPgConfigName() = "pgMapCfg2";
    config.ports()[1].pfc() = pfc;

    EXPECT_THROW(
        publishAndApplyConfig(stateV6, &config, platform.get()), FbossError);

    // Fix pgMapCfg2 to use bufferPool1 and check again
    createPortPgConfig(
        0 /* pg start index */,
        kStateTestNumPortPgs /* pg end index */,
        "pgMapCfg2",
        "bufferPool1");

    EXPECT_NO_THROW(publishAndApplyConfig(stateV6, &config, platform.get()));
  }
}
