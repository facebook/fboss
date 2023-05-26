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

facebook::fboss::HwSwitchMatcher scope() {
  return facebook::fboss::HwSwitchMatcher{
      std::unordered_set<facebook::fboss::SwitchID>{
          facebook::fboss::SwitchID(0)}};
}
} // namespace

using namespace facebook::fboss;

using UdfGroupCfgList = std::vector<cfg::UdfGroup>;
using UdfPacketMatcherCfgList = std::vector<cfg::UdfPacketMatcher>;
using UdfGroupIds = std::vector<std::string>;

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
  auto switchSettings = getFirstNodeIf(state->getMultiSwitchSwitchSettings());
  switchSettings = switchSettings->clone();
  switchSettings->setUdfConfig(udfConfig);
  auto multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(scope().matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);

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
  auto serialized = udfEntry->toThrift();
  auto deserialized = std::make_shared<UdfGroup>(serialized);
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

  auto serialized = udfStatePacketmatcherEntry->toThrift();
  auto deserialized = std::make_shared<UdfPacketMatcher>(serialized);

  EXPECT_TRUE(*udfStatePacketmatcherEntry == *deserialized);
}

TEST(Udf, addUpdate) {
  auto state = std::make_shared<SwitchState>();

  cfg::UdfConfig udf;
  auto udfConfig = std::make_shared<UdfConfig>();
  // convert to state
  udfConfig->fromThrift(udf);

  // update the state with udfCfg
  auto switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setUdfConfig(udfConfig);
  auto multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(scope().matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);

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
  addSwitchInfo(stateV0);

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
  auto switchSettingsV2 =
      getFirstNodeIf(stateV2->getMultiSwitchSwitchSettings());
  EXPECT_EQ(stateV2->getUdfConfig(), switchSettingsV2->getUdfConfig());
  const auto stateThrift = stateV2->toThrift();
  // make sure we are writing the global and switchSettings entry for
  // compatibility
  EXPECT_EQ(
      stateThrift.udfConfig(), *(stateThrift.switchSettings()->udfConfig()));

  // undo udf cfg
  config.udfConfig().reset();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  EXPECT_EQ(stateV3->getUdfConfig()->toThrift(), cfg::UdfConfig());
}

TEST(Udf, validateMissingPacketMatcherConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0);

  cfg::SwitchConfig config;
  cfg::UdfConfig udf;
  config.udfConfig() = udf;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_EQ(nullptr, stateV1);
  // no map has been populated
  EXPECT_EQ(stateV0->getUdfConfig()->getUdfGroupMap()->size(), 0);

  auto udfEntry = makeCfgUdfGroupEntry(kUdfGroupCfgName1.str());
  // Add matchCfg_2 for packetMatcher while matchCfg_1 is associated with
  // udfGroup
  auto udfPacketmatcherEntry =
      makeCfgUdfPacketMatcherEntry(kPacketMatcherCfgName2.str());
  config.udfConfig() = makeUdfCfg({udfEntry}, {udfPacketmatcherEntry});

  // As there is configuartion mismatch. It would throw following exception
  // "Configuration does not exist for UdfPacketMatcherMap: matchCfg_1
  // but exists in packetMatcherList for UdfGroup foo1"
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

cfg::LoadBalancer makeLoadBalancerCfg(UdfGroupIds udfGroupIds) {
  cfg::LoadBalancer loadBalancer;
  cfg::Fields fields;

  loadBalancer.id() = LoadBalancerID::ECMP;

  std::set<cfg::IPv4Field> v4Fields = {
      cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS};
  fields.ipv4Fields() = std::move(v4Fields);

  std::set<cfg::IPv6Field> v6Fields = {
      cfg::IPv6Field::SOURCE_ADDRESS, cfg::IPv6Field::DESTINATION_ADDRESS};
  fields.ipv6Fields() = std::move(v6Fields);

  std::set<cfg::MPLSField> mplsFields = {
      LoadBalancer::MPLSField::TOP_LABEL,
      LoadBalancer::MPLSField::SECOND_LABEL,
      LoadBalancer::MPLSField::THIRD_LABEL};
  fields.mplsFields() = std::move(mplsFields);

  std::set<cfg::TransportField> transportFields = {
      cfg::TransportField::SOURCE_PORT, cfg::TransportField::DESTINATION_PORT};
  fields.transportFields() = std::move(transportFields);

  fields.udfGroups() = udfGroupIds;
  loadBalancer.fieldSelection() = fields;

  return loadBalancer;
}

TEST(Udf, validateMissingUdfGroupConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();

  cfg::SwitchConfig config;
  cfg::LoadBalancer loadBalancer;
  cfg::UdfConfig udf;

  auto udfEntry = makeCfgUdfGroupEntry(kUdfGroupCfgName1.str());
  auto udfPacketmatcherEntry =
      makeCfgUdfPacketMatcherEntry(kPacketMatcherCfgName.str());

  // Add UdfGroup foo2 in LoadBalancer UdfGroupList
  // udfGroup, though the UdfConfig exist for UdfGroup foo1
  loadBalancer = makeLoadBalancerCfg({kUdfGroupCfgName2.str()});
  config.loadBalancers() = {loadBalancer};
  config.udfConfig() = makeUdfCfg({udfEntry}, {udfPacketmatcherEntry});

  // As there is configuartion mismatch. It would throw following exception
  // "Configuration does not exist for UdfGroup: foo2 but exists in UdfGroupList
  // for LoadBalancer 0"
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(Udf, removeUdfConfigStateDelta) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0);

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
  EXPECT_EQ(stateV3->getUdfConfig()->toThrift(), cfg::UdfConfig());

  StateDelta delta(stateV2, stateV3);
  std::set<std::string> foundRemovedUdfGroup;
  DeltaFunctions::forEachRemoved(
      delta.getUdfGroupDelta(), [&](const std::shared_ptr<UdfGroup>& udfGroup) {
        auto ret = foundRemovedUdfGroup.insert(udfGroup->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(foundRemovedUdfGroup.size(), 1);
  EXPECT_NE(
      foundRemovedUdfGroup.find(kUdfGroupCfgName1.str()),
      foundRemovedUdfGroup.end());

  std::set<std::string> foundRemovedUdfPacketMatcher;
  DeltaFunctions::forEachRemoved(
      delta.getUdfPacketMatcherDelta(),
      [&](const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher) {
        auto ret =
            foundRemovedUdfPacketMatcher.insert(udfPacketMatcher->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(foundRemovedUdfPacketMatcher.size(), 1);
  EXPECT_NE(
      foundRemovedUdfPacketMatcher.find(kPacketMatcherCfgName.str()),
      foundRemovedUdfPacketMatcher.end());
}
