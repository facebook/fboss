/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/UdfConfig.h"
#include "fboss/agent/state/UdfGroupMap.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

std::shared_ptr<UdfGroup> createStateUdfGroup(
    const std::string& name,
    int fieldSize = 0) {
  cfg::UdfGroup udfGroupEntry;
  cfg::UdfPacketMatcher matchCfg;

  matchCfg.name() = "matchCfg_1";
  matchCfg.l4PktType() = cfg::UdfMatchL4Type::UDF_L4_PKT_TYPE_UDP;

  udfGroupEntry.name() = name;
  udfGroupEntry.header() = cfg::UdfBaseHeaderType::UDF_L4_HEADER;
  udfGroupEntry.startOffsetInBytes() = 5;
  udfGroupEntry.fieldSizeInBytes() = fieldSize;
  udfGroupEntry.udfPacketMatcherIds() = {"matchCfg_1"};

  auto udfEntry = std::make_shared<UdfGroup>(name);
  udfEntry->fromThrift(udfGroupEntry);
  return udfEntry;
}

cfg::UdfGroup makeCfgUdfGroupEntry(std::string name, int fieldSize = 0) {
  cfg::UdfGroup udfGroupEntry;

  udfGroupEntry.name() = name;
  udfGroupEntry.header() = cfg::UdfBaseHeaderType::UDF_L4_HEADER;
  udfGroupEntry.startOffsetInBytes() = 5;
  udfGroupEntry.fieldSizeInBytes() = fieldSize;
  udfGroupEntry.udfPacketMatcherIds() = {"matchCfg_1"};

  return udfGroupEntry;
}

cfg::UdfConfig makeUdfCfg(std::vector<cfg::UdfGroup> udfGroupList) {
  cfg::UdfConfig udf;
  cfg::UdfGroup udfGroupEntry;
  std::map<std::string, cfg::UdfGroup> udfMap;

  for (const auto& udfGroupEntry : udfGroupList) {
    std::string name = *udfGroupEntry.name();
    udfMap.insert(std::make_pair(name, udfGroupEntry));
  }
  udf.udfGroups() = udfMap;
  return udf;
}

TEST(Udf, addRemove) {
  auto state = std::make_shared<SwitchState>();
  auto udfEntry1 = makeCfgUdfGroupEntry("foo1");
  auto udfEntry2 = makeCfgUdfGroupEntry("foo2");

  cfg::UdfConfig udf = makeUdfCfg({udfEntry1, udfEntry2});
  auto udfConfig = std::make_shared<UdfConfig>();
  udfConfig->fromThrift(udf);
  // update the state
  state->resetUdfConfig(udfConfig);

  // both entries should be present
  EXPECT_EQ(state->getUdfConfig()->getUdfGroupMap()->size(), 2);

  state->getUdfConfig()->getUdfGroupMap()->removeNode("foo1");
  state->getUdfConfig()->getUdfGroupMap()->removeNode("foo2");

  EXPECT_EQ(state->getUdfConfig()->getUdfGroupMap()->size(), 0);
  EXPECT_EQ(state->getUdfConfig()->getUdfGroupMap()->getNodeIf("foo"), nullptr);
}

TEST(Udf, serDesUdfEntries) {
  auto udfEntry = createStateUdfGroup("foo1");
  auto serialized = udfEntry->toFollyDynamic();
  auto deserialized = udfEntry->fromFollyDynamic(serialized);
  EXPECT_TRUE(*udfEntry == *deserialized);
}

TEST(Udf, addUpdate) {
  auto state = std::make_shared<SwitchState>();

  cfg::UdfConfig udf;
  auto udfConfig = std::make_shared<UdfConfig>();
  // convert to state
  udfConfig->fromThrift(udf);

  // update the state with udfCfg
  state->resetUdfConfig(udfConfig);

  EXPECT_EQ(state->getUdfConfig()->getUdfGroupMap()->size(), 0);

  auto udfEntry = createStateUdfGroup("foo");
  state->getUdfConfig()->getUdfGroupMap()->addNode(udfEntry);

  EXPECT_EQ(
      *udfEntry, *state->getUdfConfig()->getUdfGroupMap()->getNode("foo"));

  auto udfGroupMap = state->getUdfConfig()->getUdfGroupMap();
  for (const auto& udfGroupEntry : *udfGroupMap) {
    auto udfGroup = udfGroupEntry.second;
    EXPECT_EQ(udfGroup->getUdfPacketMatcherIds().size(), 1);
    for (const auto& matcherId : udfGroup->getUdfPacketMatcherIds()) {
      EXPECT_EQ(matcherId, "matchCfg_1");
    }
  }

  auto udfEntry_1 = createStateUdfGroup("foo", 4);
  state->getUdfConfig()->getUdfGroupMap()->updateNode(udfEntry_1);
  // validate that update takes effect by comparing with old version
  EXPECT_NE(
      *udfEntry, *state->getUdfConfig()->getUdfGroupMap()->getNode("foo"));
}

TEST(Udf, publish) {
  cfg::UdfConfig udfCfg;
  auto udfConfig = std::make_shared<UdfConfig>();
  auto state = std::make_shared<SwitchState>();

  // convert to state
  udfConfig->fromThrift(udfCfg);
  // update the state with udfCfg
  state->publish();
  EXPECT_TRUE(state->getUdfConfig()->isPublished());
}

TEST(Udf, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();

  cfg::SwitchConfig config;
  cfg::UdfConfig udf;
  config.udfConfig() = udf;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_EQ(nullptr, stateV1);
  // no map has been populated
  EXPECT_EQ(stateV0->getUdfConfig()->getUdfGroupMap()->size(), 0);

  auto udfEntry = makeCfgUdfGroupEntry("foo1");
  config.udfConfig() = makeUdfCfg({udfEntry});
  auto stateV2 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);

  // one entry has been added <foo1>
  EXPECT_EQ(stateV2->getUdfConfig()->getUdfGroupMap()->size(), 1);
}
