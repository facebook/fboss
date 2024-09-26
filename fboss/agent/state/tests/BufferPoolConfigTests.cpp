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
// #include "fboss/agent/state/Port.h"
// #include "fboss/agent/state/PortQueue.h"
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

TEST(BufferPoolConfigTest, bufferPoolName) {
  int pgId = 0;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  std::vector<cfg::PortPgConfig> portPgConfigs;
  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    portPgConfigs.emplace_back(pgConfig);
  }
  portPgConfigMap["foo"] = portPgConfigs;

  cfg::PortPfc pfc;
  pfc.portPgConfigName() = "foo";
  config.ports()[0].pfc() = pfc;
  config.portPgConfigs() = portPgConfigMap;

  portPgConfigs.clear();
  for (pgId = 0; pgId < kStateTestNumPortPgs; pgId++) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    pgConfig.name() = folly::to<std::string>("pg", pgId);
    pgConfig.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
    pgConfig.minLimitBytes() = 1000;
    pgConfig.resumeOffsetBytes() = 100;
    pgConfig.bufferPoolName() = "bufferNew";
    portPgConfigs.emplace_back(pgConfig);
  }

  portPgConfigMap["foo"] = portPgConfigs;
  config.portPgConfigs() = portPgConfigMap;

  // configured bufferName but no buffer map exists.
  // Throw execpetion
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);

  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
  {
    cfg::BufferPoolConfig tmpPoolConfig;
    bufferPoolCfgMap.insert(make_pair("bufferOld", tmpPoolConfig));
    config.bufferPoolConfigs() = bufferPoolCfgMap;

    // configured "bufferName" buffer map exists.
    // but "bufferNew" doesn't exist. Only "bufferOld" does so expect an error
    EXPECT_THROW(
        publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
  }

  {
    cfg::BufferPoolConfig tmpPoolConfig;
    bufferPoolCfgMap.insert(make_pair("bufferNew", tmpPoolConfig));
    config.bufferPoolConfigs() = bufferPoolCfgMap;

    // update goes through now, as "bufferNew" is configured
    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);
  }

  {
    // unconfigure the PFC
    config.ports()[0].pfc().reset();

    // buffer pool can have any entry, shouldn't matter
    cfg::BufferPoolConfig tmpPoolConfig;
    bufferPoolCfgMap.insert(make_pair("coolBuffer", tmpPoolConfig));
    config.bufferPoolConfigs() = bufferPoolCfgMap;

    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);
  }
}

TEST(BufferPoolConfigTest, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  const int kHeadroomBytes = 1000;
  const int kSharedBytes = 1500;

  cfg::SwitchConfig config;
  EXPECT_NE(nullptr, stateV0->getBufferPoolCfgs());
  EXPECT_EQ(0, stateV0->getBufferPoolCfgs()->numNodes());
  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;

  auto updateBufferPoolCfg = [&](const std::string& key,
                                 const int sharedBytes,
                                 const int headroomBytes) {
    cfg::BufferPoolConfig tmpPoolConfig;
    tmpPoolConfig.headroomBytes() = headroomBytes;
    tmpPoolConfig.sharedBytes() = sharedBytes;
    bufferPoolCfgMap.insert(make_pair(key, tmpPoolConfig));
  };

  // add entries into the buffer pool
  updateBufferPoolCfg("bufferPool_0", kSharedBytes, kHeadroomBytes);
  updateBufferPoolCfg("bufferPool_1", kSharedBytes, kHeadroomBytes);

  config.bufferPoolConfigs() = bufferPoolCfgMap;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto bufferPools = stateV1->getBufferPoolCfgs();

  EXPECT_NE(nullptr, bufferPools);
  EXPECT_EQ(2, (*bufferPools).numNodes());

  int index = 0;
  for (auto mMapIter : std::as_const(*bufferPools)) {
    for (auto iter : std::as_const(*mMapIter.second)) {
      auto bufferPool = iter.second;
      std::string bufferPoolName = folly::to<std::string>("bufferPool_", index);
      EXPECT_EQ(bufferPool->getID(), bufferPoolName);
      EXPECT_EQ(bufferPool->getSharedBytes(), kSharedBytes);
      EXPECT_EQ(*bufferPool->getHeadroomBytes(), kHeadroomBytes);
      index++;
    }
  }

  {
    // push same  contents of the existing bufferPool, ensure no change
    bufferPoolCfgMap.clear();

    // add same entries to the buffer pool
    updateBufferPoolCfg("bufferPool_0", kSharedBytes, kHeadroomBytes);
    updateBufferPoolCfg("bufferPool_1", kSharedBytes, kHeadroomBytes);
    config.bufferPoolConfigs() = bufferPoolCfgMap;
    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_EQ(nullptr, stateV2);
  }

  {
    // modify the contents of the existing bufferPool, bump up
    bufferPoolCfgMap.clear();

    // add new entries to the buffer pool
    updateBufferPoolCfg("bufferPool_0", kSharedBytes, kHeadroomBytes + 1);
    updateBufferPoolCfg("bufferPool_1", kSharedBytes, kHeadroomBytes);
    config.bufferPoolConfigs() = bufferPoolCfgMap;
    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_NE(nullptr, stateV2);
  }

  {
    // add 1 more buffer pool entry, so cfg change
    // should happen, size of map changes
    updateBufferPoolCfg("bufferPool_2", kSharedBytes, kHeadroomBytes);
    config.bufferPoolConfigs() = bufferPoolCfgMap;
    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_NE(nullptr, stateV2);

    auto bufferPoolCfgs = stateV2->getBufferPoolCfgs();
    EXPECT_NE(nullptr, bufferPoolCfgs);
    EXPECT_EQ(bufferPoolCfgMap.size(), (*bufferPoolCfgs).numNodes());
  }

  config.bufferPoolConfigs().reset();
  auto stateEnd = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateEnd);
  bufferPools = stateEnd->getBufferPoolCfgs();
  EXPECT_EQ(0, bufferPools->numNodes());
}
