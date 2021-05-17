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
#include "fboss/agent/state/BufferPoolConfig.h"
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

  // pg_id should be [0, PORT_PG_VALUE_MAX]
  // Lets exceed more than PG_MAX
  for (pgId = 0; pgId <= cfg::switch_config_constants::PORT_PG_VALUE_MAX() + 1;
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

TEST(PortPgConfig, applyConfig) {
  int pgId = 0;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  constexpr folly::StringPiece kBufferPoolName = "bufferPool";
  constexpr folly::StringPiece kPgConfigName = "foo";

  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
  cfg::SwitchConfig config;
  config.ports_ref()->resize(2);
  config.ports_ref()[0].logicalID_ref() = 1;
  config.ports_ref()[0].name_ref() = "port1";
  config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;

  config.ports_ref()[1].logicalID_ref() = 2;
  config.ports_ref()[1].name_ref() = "port2";
  config.ports_ref()[1].state_ref() = cfg::PortState::ENABLED;

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;
  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id_ref() = pgId;
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap[kPgConfigName.str()] = portPgConfigs;

  cfg::PortPfc pfc;
  pfc.portPgConfigName_ref() = kPgConfigName.str();
  config.ports_ref()[0].pfc_ref() = pfc;
  config.portPgConfigs_ref() = portPgConfigMap;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto pgCfgs = stateV1->getPort(PortID(1))->getPortPgConfigs();
  EXPECT_TRUE(pgCfgs);

  auto pgCfgValue = pgCfgs.value();
  EXPECT_EQ(kStateTestNumPortPgs, pgCfgValue.size());

  auto createPortPgConfig = [&](const int pgIdStart,
                                const int pgIdEnd,
                                const std::string& pgConfigName,
                                const std::string& bufferName) {
    portPgConfigs.clear();
    for (pgId = pgIdStart; pgId < pgIdEnd; pgId++) {
      cfg::PortPgConfig pgConfig;
      pgConfig.id_ref() = pgId;
      pgConfig.name_ref() = folly::to<std::string>("pg", pgId);
      pgConfig.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
      pgConfig.minLimitBytes_ref() = 1000;
      pgConfig.resumeOffsetBytes_ref() = 100;
      pgConfig.bufferPoolName_ref() = bufferName;
      portPgConfigs.emplace_back(pgConfig);
    }
    portPgConfigMap[pgConfigName] = portPgConfigs;
    config.portPgConfigs_ref() = portPgConfigMap;
  };

  auto createBufferPoolCfg = [&](const int sharedBytes,
                                 const int headroomBytes,
                                 const std::string& bufferName) {
    cfg::BufferPoolConfig tmpPoolConfig1;
    tmpPoolConfig1.headroomBytes_ref() = headroomBytes;
    tmpPoolConfig1.sharedBytes_ref() = sharedBytes;
    bufferPoolCfgMap.insert(make_pair(bufferName, tmpPoolConfig1));
    config.bufferPoolConfigs_ref() = bufferPoolCfgMap;
  };

  auto validateBufferPoolCfg = [&](const int sharedBytes,
                                   const int headroomBytes,
                                   const auto& pgCfgsNew) {
    EXPECT_TRUE(pgCfgsNew);
    for (const auto& pgCfg : *pgCfgsNew) {
      auto bufferPoolCfgPtr = pgCfg->getBufferPoolConfig();
      EXPECT_EQ((*bufferPoolCfgPtr)->getID(), kBufferPoolName.str());
      EXPECT_EQ((*bufferPoolCfgPtr)->getSharedBytes(), sharedBytes);
      EXPECT_EQ((*bufferPoolCfgPtr)->getHeadroomBytes(), headroomBytes);
    }
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
  config.bufferPoolConfigs_ref() = bufferPoolCfgMap;

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
    EXPECT_EQ(pgCfgValue[pgId]->getBufferPoolName(), kBufferPoolName.str());
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
    pgCfgValue = pgCfgsNew.value();
    EXPECT_EQ(
        cfg::switch_config_constants::PORT_PG_VALUE_MAX(), pgCfgValue.size());
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
    pgCfgValue = pgCfgsNew.value();
    EXPECT_EQ(kStateTestNumPortPgs, pgCfgValue.size());

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
    validateBufferPoolCfg(kBufferSharedBytes, kBufferHdrmBytes, pgCfgsNew);

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
    validateBufferPoolCfg(
        kBufferSharedBytes + kDelta, kBufferHdrmBytes + kDelta, pgCfgsNew1);

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
    config.bufferPoolConfigs_ref() = bufferPoolCfgMap;
    stateV5 = publishAndApplyConfig(stateV4, &config, platform.get());
    EXPECT_NE(nullptr, stateV5);
    const auto& pgCfgsNew2 = stateV5->getPort(PortID(1))->getPortPgConfigs();
    for (const auto& pgCfg : *pgCfgsNew2) {
      EXPECT_FALSE(pgCfg->getBufferPoolConfig());
    }
  }

  // undo the changes in the port pfc struct, now all is cleaned
  config.portPgConfigs_ref().reset();
  config.ports_ref()[0].pfc_ref().reset();
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

    pfc.portPgConfigName_ref() = "pgMapCfg1";
    config.ports_ref()[0].pfc_ref() = pfc;

    pfc.portPgConfigName_ref() = "pgMapCfg2";
    config.ports_ref()[1].pfc_ref() = pfc;

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
