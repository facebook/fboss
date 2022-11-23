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
#include "fboss/agent/state/UdfGroup.h"
#include "fboss/agent/state/UdfGroupMap.h"
#include "fboss/agent/state/UdfPacketMatcher.h"
#include "fboss/agent/state/UdfPacketMatcherMap.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

namespace {
constexpr folly::StringPiece kPacketMatcherCfgName = "matchCfg_1";
constexpr folly::StringPiece kPacketMatcherCfgName2 = "matchCfg_2";
constexpr folly::StringPiece kUdfGroupCfgName1 = "foo1";
constexpr folly::StringPiece kUdfGroupCfgName2 = "foo2";
} // namespace

using namespace facebook::fboss;

using UdfGroupCfgList = std::vector<cfg::UdfGroup>;
using UdfPacketMatcherCfgList = std::vector<cfg::UdfPacketMatcher>;

std::shared_ptr<UdfGroup> createStateUdfGroup(
    const std::string& name,
    int fieldSize = 0) {
  cfg::UdfGroup udfGroupEntry;

  udfGroupEntry.name() = name;
  udfGroupEntry.header() = cfg::UdfBaseHeaderType::UDF_L4_HEADER;
  udfGroupEntry.startOffsetInBytes() = 5;
  udfGroupEntry.fieldSizeInBytes() = fieldSize;
  udfGroupEntry.udfPacketMatcherIds() = {kPacketMatcherCfgName.str()};

  auto udfEntry = std::make_shared<UdfGroup>(name);
  udfEntry->fromThrift(udfGroupEntry);
  return udfEntry;
}

cfg::UdfPacketMatcher makeCfgUdfPacketMatcherEntry(
    std::string name,
    int dstPort = 0) {
  cfg::UdfPacketMatcher udfPkt;

  udfPkt.name() = name;
  udfPkt.l2PktType() = cfg::UdfMatchL2Type::UDF_L2_PKT_TYPE_ETH;
  udfPkt.l3pktType() = cfg::UdfMatchL3Type::UDF_L3_PKT_TYPE_IPV6;

  udfPkt.UdfL4DstPort() = dstPort;
  return udfPkt;
}

cfg::UdfGroup makeCfgUdfGroupEntry(std::string name, int fieldSize = 0) {
  cfg::UdfGroup udfGroupEntry;

  udfGroupEntry.name() = name;
  udfGroupEntry.header() = cfg::UdfBaseHeaderType::UDF_L4_HEADER;
  udfGroupEntry.startOffsetInBytes() = 5;
  udfGroupEntry.fieldSizeInBytes() = fieldSize;
  udfGroupEntry.udfPacketMatcherIds() = {kPacketMatcherCfgName.str()};

  return udfGroupEntry;
}

cfg::UdfConfig makeUdfCfg(
    UdfGroupCfgList udfGroupList,
    UdfPacketMatcherCfgList udfPacketMatcherList) {
  cfg::UdfConfig udf;
  cfg::UdfGroup udfGroupEntry;
  std::map<std::string, cfg::UdfGroup> udfMap;
  std::map<std::string, cfg::UdfPacketMatcher> udfPacketMatcherMap;

  for (const auto& udfGroup : udfGroupList) {
    std::string name = *udfGroup.name();
    udfMap.insert(std::make_pair(name, udfGroup));
  }
  for (const auto& udfPacketMatcherEntry : udfPacketMatcherList) {
    std::string name = *udfPacketMatcherEntry.name();
    udfPacketMatcherMap.insert(std::make_pair(name, udfPacketMatcherEntry));
  }
  udf.udfGroups() = udfMap;
  udf.udfPacketMatcher() = udfPacketMatcherMap;
  return udf;
}

TEST(Udf, addUpdateRemove) {
  auto state = std::make_shared<SwitchState>();
  auto udfEntry1 = makeCfgUdfGroupEntry(kUdfGroupCfgName1.str());
  auto udfEntry2 = makeCfgUdfGroupEntry(kUdfGroupCfgName2.str());

  auto udfPacketmatcherEntry =
      makeCfgUdfPacketMatcherEntry(kPacketMatcherCfgName.str());
  // convert to state object
  const auto pktMatcherCfgName = kPacketMatcherCfgName.str();
  auto udfStatePacketmatcherEntry =
      std::make_shared<UdfPacketMatcher>(pktMatcherCfgName);
  udfStatePacketmatcherEntry->fromThrift(udfPacketmatcherEntry);

  cfg::UdfConfig udf =
      makeUdfCfg({udfEntry1, udfEntry2}, {udfPacketmatcherEntry});
  auto udfConfig = std::make_shared<UdfConfig>();
  udfConfig->fromThrift(udf);

  // update the state
  state->resetUdfConfig(udfConfig);

  // both entries should be present
  EXPECT_EQ(state->getUdfConfig()->getUdfGroupMap()->size(), 2);
  EXPECT_EQ(state->getUdfConfig()->getUdfPacketMatcherMap()->size(), 1);

  EXPECT_EQ(
      *state->getUdfConfig()->getUdfPacketMatcherMap()->getNodeIf(
          kPacketMatcherCfgName.str()),
      *udfStatePacketmatcherEntry);

  // add another entry for pktMatcher
  const auto pktMatcherCfgName2 = kPacketMatcherCfgName2.str();
  auto udfPacketmatcherEntry2 =
      makeCfgUdfPacketMatcherEntry(kPacketMatcherCfgName2.str());
  auto udfStatePacketmatcherEntry2 =
      std::make_shared<UdfPacketMatcher>(pktMatcherCfgName2);
  udfStatePacketmatcherEntry2->fromThrift(udfPacketmatcherEntry2);
  state->getUdfConfig()->getUdfPacketMatcherMap()->addNode(
      udfStatePacketmatcherEntry2);

  EXPECT_EQ(state->getUdfConfig()->getUdfPacketMatcherMap()->size(), 2);

  // update the second packet matcher entry
  auto udfPacketmatcherEntryModified2 =
      makeCfgUdfPacketMatcherEntry(kPacketMatcherCfgName2.str(), 200);
  udfStatePacketmatcherEntry2->fromThrift(udfPacketmatcherEntryModified2);
  state->getUdfConfig()->getUdfPacketMatcherMap()->updateNode(
      udfStatePacketmatcherEntry2);
  EXPECT_EQ(state->getUdfConfig()->getUdfPacketMatcherMap()->size(), 2);

  // update first udfGroup entry as well
  auto udfEntryUpdated1 = createStateUdfGroup(kUdfGroupCfgName1.str(), 4);
  state->getUdfConfig()->getUdfGroupMap()->updateNode(udfEntryUpdated1);
  // validate that update takes effect by comparing contents
  EXPECT_EQ(
      *udfEntryUpdated1,
      *state->getUdfConfig()->getUdfGroupMap()->getNode(
          kUdfGroupCfgName1.str()));

  // remove nodes
  state->getUdfConfig()->getUdfGroupMap()->removeNode(kUdfGroupCfgName1.str());
  state->getUdfConfig()->getUdfGroupMap()->removeNode(kUdfGroupCfgName2.str());
  state->getUdfConfig()->getUdfPacketMatcherMap()->removeNode(
      kPacketMatcherCfgName2.str());

  EXPECT_EQ(state->getUdfConfig()->getUdfGroupMap()->size(), 0);
  EXPECT_EQ(state->getUdfConfig()->getUdfPacketMatcherMap()->size(), 1);
  EXPECT_EQ(
      state->getUdfConfig()->getUdfGroupMap()->getNodeIf(
          kUdfGroupCfgName1.str()),
      nullptr);
  EXPECT_EQ(
      state->getUdfConfig()->getUdfGroupMap()->getNodeIf(
          kPacketMatcherCfgName2.str()),
      nullptr);
}

TEST(Udf, serDesUdfGroup) {
  auto udfEntry = createStateUdfGroup(kUdfGroupCfgName1.str());
  auto serialized = udfEntry->toFollyDynamic();
  auto deserialized = UdfGroup::fromFollyDynamic(serialized);
  EXPECT_TRUE(*udfEntry == *deserialized);
}

TEST(Udf, serDesUdfPacketMatcher) {
  auto udfPacketmatcherEntry =
      makeCfgUdfPacketMatcherEntry(kPacketMatcherCfgName.str());

  // convert to state object
  const auto pktMatcherCfgName = kPacketMatcherCfgName.str();
  auto udfStatePacketmatcherEntry =
      std::make_shared<UdfPacketMatcher>(pktMatcherCfgName);
  udfStatePacketmatcherEntry->fromThrift(udfPacketmatcherEntry);

  auto serialized = udfStatePacketmatcherEntry->toFollyDynamic();
  auto deserialized = UdfPacketMatcher::fromFollyDynamic(serialized);

  EXPECT_TRUE(*udfStatePacketmatcherEntry == *deserialized);
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

  auto udfEntry = createStateUdfGroup(kUdfGroupCfgName1.str());
  state->getUdfConfig()->getUdfGroupMap()->addNode(udfEntry);

  EXPECT_EQ(
      *udfEntry,
      *state->getUdfConfig()->getUdfGroupMap()->getNode(
          kUdfGroupCfgName1.str()));

  auto udfGroupMap = state->getUdfConfig()->getUdfGroupMap();
  for (const auto& udfGroupEntry : *udfGroupMap) {
    auto udfGroup = udfGroupEntry.second;
    EXPECT_EQ(udfGroup->getUdfPacketMatcherIds().size(), 1);
    for (const auto& matcherId : udfGroup->getUdfPacketMatcherIds()) {
      EXPECT_EQ(matcherId, kPacketMatcherCfgName.str());
    }
  }

  auto udfEntry_1 = createStateUdfGroup(kUdfGroupCfgName1.str(), 4);
  state->getUdfConfig()->getUdfGroupMap()->updateNode(udfEntry_1);
  // validate that update takes effect by comparing with old version
  EXPECT_NE(
      *udfEntry,
      *state->getUdfConfig()->getUdfGroupMap()->getNode(
          kUdfGroupCfgName1.str()));
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

  auto udfEntry = makeCfgUdfGroupEntry(kUdfGroupCfgName1.str());
  auto udfPacketmatcherEntry =
      makeCfgUdfPacketMatcherEntry(kPacketMatcherCfgName.str());

  // add udfGroup, udfPacketMatcher entries to the udf cfg
  config.udfConfig() = makeUdfCfg({udfEntry}, {udfPacketmatcherEntry});
  auto stateV2 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);

  // one entry has been added <foo1>
  EXPECT_EQ(stateV2->getUdfConfig()->getUdfGroupMap()->size(), 1);
  // one entry has been added <matchCfg_1>
  EXPECT_EQ(stateV2->getUdfConfig()->getUdfPacketMatcherMap()->size(), 1);

  // undo udf cfg
  config.udfConfig().reset();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  EXPECT_EQ(stateV3->getUdfConfig(), nullptr);
}
