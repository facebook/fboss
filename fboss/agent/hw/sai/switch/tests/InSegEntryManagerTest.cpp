// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/Singleton.h>

namespace facebook::fboss {

class InSegEntryManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    for (auto& intf : testInterfaces) {
      auto arpEntry = makeArpEntry(intf.id, intf.remoteHosts[0]);
      saiManagerTable->neighborManager().addNeighbor(arpEntry);
    }
  }

  LabelNextHopEntry getLabelNextHopEntryWithNextHops(
      int fromNextHop,
      int toNextHop,
      LabelForwardingAction::LabelForwardingType type) {
    LabelNextHopSet nexthops;
    for (auto nextHopIndex = fromNextHop;
         nextHopIndex < toNextHop && nextHopIndex < testInterfaces.size();
         nextHopIndex++) {
      auto ip = testInterfaces[nextHopIndex].remoteHosts[0].ip;
      nexthops.emplace(makeMplsNextHop(
          testInterfaces[nextHopIndex],
          getActionForNextHop(type, nextHopIndex)));
    }
    return LabelNextHopEntry(
        std::move(nexthops), AdminDistance::MAX_ADMIN_DISTANCE);
  }

  LabelForwardingAction getActionForNextHop(
      LabelForwardingAction::LabelForwardingType type,
      int nextHopIndex) {
    switch (type) {
      case LabelForwardingAction::LabelForwardingType::PUSH:
        return LabelForwardingAction{
            type, {100 + nextHopIndex, 200 + nextHopIndex, 300 + nextHopIndex}};

      case LabelForwardingAction::LabelForwardingType::SWAP:
        return LabelForwardingAction{type, 1000 + nextHopIndex};

      case LabelForwardingAction::LabelForwardingType::PHP:
      case LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP:
      case LabelForwardingAction::LabelForwardingType::NOOP:
        break;
    }
    return LabelForwardingAction{type};
  }

  void addEntryToLabelForwardingInformationBase(
      LabelForwardingInformationBase* fib,
      LabelForwardingEntry::Label label,
      int nextHopBegin,
      int nextHopEnd,
      LabelForwardingAction::LabelForwardingType actionType) {
    fib->addNode(std::make_shared<LabelForwardingEntry>(
        label,
        ClientID(ClientID::OPENR),
        getLabelNextHopEntryWithNextHops(
            nextHopBegin, nextHopEnd, actionType)));
  }

  void removeEntryFromLabelForwardingInformationBase(
      LabelForwardingInformationBase* fib,
      LabelForwardingEntry::Label label) {
    fib->removeNode(label);
  }

  void verifyLabelForwardingEntry(
      LabelForwardingEntry::Label label,
      int nextHopBegin,
      int nextHopEnd) {
    const auto* inSegEntryHandle =
        saiManagerTable->inSegEntryManager().getInSegEntryHandle(label);
    ASSERT_NE(inSegEntryHandle, nullptr);
    ASSERT_NE(inSegEntryHandle->inSegEntry, nullptr);
    ASSERT_NE(inSegEntryHandle->nextHopGroupHandle, nullptr);

    auto adapterKey = inSegEntryHandle->inSegEntry->adapterKey();
    EXPECT_EQ(adapterKey.label(), label);
    auto switchId = saiManagerTable->switchManager().getSwitchSaiId();
    EXPECT_EQ(adapterKey.switchId(), switchId);

    auto attributes = inSegEntryHandle->inSegEntry->attributes();
    EXPECT_EQ(
        GET_ATTR(InSeg, PacketAction, attributes), SAI_PACKET_ACTION_FORWARD);
    EXPECT_EQ(GET_ATTR(InSeg, NumOfPop, attributes), 1);
    auto nexHopId =
        inSegEntryHandle->nextHopGroupHandle->nextHopGroup->adapterKey();
    EXPECT_EQ(GET_OPT_ATTR(InSeg, NextHopId, attributes), nexHopId);

    verifyNextHopGroup(nexHopId, nextHopBegin, nextHopEnd);
  }

  void verifyNextHopGroup(
      NextHopGroupSaiId nextHopGroupId,
      int nextHopBegin,
      int nextHopEnd) {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    auto& nextHopApi = saiApiTable->nextHopApi();
    SaiNextHopGroupTraits::Attributes::NextHopMemberList memberList{};
    auto members = nextHopGroupApi.getAttribute(nextHopGroupId, memberList);
    EXPECT_EQ(members.size(), nextHopEnd - nextHopBegin);

    std::unordered_set<folly::IPAddress> gotNextHopIps;
    for (const auto& member : members) {
      auto nextHopId = nextHopGroupApi.getAttribute(
          NextHopGroupMemberSaiId(member),
          SaiNextHopGroupMemberTraits::Attributes::NextHopId{});
      folly::IPAddress ip = nextHopApi.getAttribute(
          NextHopSaiId(nextHopId), SaiIpNextHopTraits::Attributes::Ip{});
      gotNextHopIps.insert(ip);
    }

    for (auto nextHopIndex = nextHopBegin;
         nextHopIndex < nextHopEnd && nextHopIndex < testInterfaces.size();
         nextHopIndex++) {
      auto itr =
          gotNextHopIps.find(testInterfaces[nextHopIndex].remoteHosts[0].ip);
      EXPECT_TRUE(itr != gotNextHopIps.end());
    }
  }
};

TEST_F(InSegEntryManagerTest, createInSegEntry) {
  LabelForwardingInformationBase empty{};
  LabelForwardingInformationBase fib{};

  // setup
  addEntryToLabelForwardingInformationBase(
      &fib,
      100 /* label */,
      0 /* begin next hop id */,
      4 /* end next hop id */,
      LabelForwardingAction::LabelForwardingType::SWAP);
  NodeMapDelta<LabelForwardingInformationBase> delta(&empty, &fib);
  saiManagerTable->inSegEntryManager().processInSegEntryDelta(delta);

  // verify
  verifyLabelForwardingEntry(
      100 /* label */, 0 /* begin next hop id */, 4 /* end next hop id */);
  const auto* preWarmBootHandle =
      saiManagerTable->inSegEntryManager().getInSegEntryHandle(100);
  auto preWarmBootAdapterKey = preWarmBootHandle->inSegEntry->adapterKey();
  auto preWarmBootAttributes = preWarmBootHandle->inSegEntry->attributes();

  // warmboot
  SaiStore::getInstance()->exitForWarmBoot();
  SaiStore::getInstance()->reload();

  // verify pre and post matches
  auto postWarmBootHandle =
      SaiStore::getInstance()->get<SaiInSegTraits>().get(preWarmBootAdapterKey);
  ASSERT_NE(postWarmBootHandle, nullptr);
  auto postWarmBootAttributes = postWarmBootHandle->attributes();
  EXPECT_EQ(preWarmBootAttributes, postWarmBootAttributes);
}

} // namespace facebook::fboss
