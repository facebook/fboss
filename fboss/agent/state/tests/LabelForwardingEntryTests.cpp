// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/Utils.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/LabelForwardingEntry.h"

#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss;

namespace {

std::array<folly::IPAddress, 2> kNextHopAddrs{
    folly::IPAddress("10.0.0.1"),
    folly::IPAddress("10.0.0.2"),
};

std::array<LabelForwardingAction::LabelStack, 2> kLabelStacks{
    LabelForwardingAction::LabelStack{1001, 1002},
    LabelForwardingAction::LabelStack{2001, 2002}};

LabelForwardingAction getSwapAction(LabelForwardingAction::Label swapWith) {
  return LabelForwardingAction(
      LabelForwardingAction::LabelForwardingType::SWAP, swapWith);
}

LabelForwardingAction getPhpAction(bool isPhp = true) {
  return LabelForwardingAction(
      isPhp ? LabelForwardingAction::LabelForwardingType::PHP
            : LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
}

LabelForwardingAction getPushAction(LabelForwardingAction::LabelStack stack) {
  return LabelForwardingAction(
      LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack));
}

LabelNextHopEntry getSwapLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i],
        InterfaceID(i + 1),
        ECMP_WEIGHT,
        getSwapAction(kLabelStacks[i][0])));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

LabelNextHopEntry getPushLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i],
        InterfaceID(i + 1),
        ECMP_WEIGHT,
        getPushAction(kLabelStacks[i])));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

LabelNextHopEntry getPhpLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i], InterfaceID(i + 1), ECMP_WEIGHT, getPhpAction()));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

LabelNextHopEntry getPopLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i],
        InterfaceID(i + 1),
        ECMP_WEIGHT,
        getPhpAction(false)));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

void testToAndFromDynamic(const std::shared_ptr<LabelForwardingEntry>& entry) {
  EXPECT_EQ(
      *entry, *LabelForwardingEntry::fromFollyDynamic(entry->toFollyDynamic()));
}

} // namespace

TEST(LabelForwardingEntryTests, ToFromDynamic) {
  std::array<std::shared_ptr<LabelForwardingEntry>, 4> entries{
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          getPhpLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          getPopLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED))};
  for (const auto& entry : entries) {
    testToAndFromDynamic(entry);
  }
}

TEST(LabelForwardingEntryTests, getEntryForClient) {
  std::map<StdClientIds, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {StdClientIds::OPENR, getSwapLabelNextHopEntry},
          {StdClientIds::BGPD, getPushLabelNextHopEntry},
          {StdClientIds::STATIC_ROUTE, getPhpLabelNextHopEntry},
          {StdClientIds::INTERFACE_ROUTE, getPopLabelNextHopEntry},
      };

  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      clientNextHopsEntry[StdClientIds::OPENR](
          AdminDistance::DIRECTLY_CONNECTED));

  for (auto stdClientId : {StdClientIds::BGPD,
                           StdClientIds::STATIC_ROUTE,
                           StdClientIds::INTERFACE_ROUTE}) {
    entry->update(
        StdClientIds2ClientID(stdClientId),
        clientNextHopsEntry[stdClientId](AdminDistance::DIRECTLY_CONNECTED));
  }

  for (const auto clientNextHop : clientNextHopsEntry) {
    auto* nexthopEntry =
        entry->getEntryForClient(StdClientIds2ClientID(clientNextHop.first));
    EXPECT_EQ(
        *nexthopEntry, clientNextHop.second(AdminDistance::DIRECTLY_CONNECTED));
  }
  EXPECT_EQ(
      nullptr,
      entry->getEntryForClient(
          StdClientIds2ClientID(StdClientIds::NETLINK_LISTENER)));
}

TEST(LabelForwardingEntryTests, delEntryForClient) {
  std::map<StdClientIds, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {StdClientIds::OPENR, getSwapLabelNextHopEntry},
          {StdClientIds::BGPD, getPushLabelNextHopEntry},
      };

  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      clientNextHopsEntry[StdClientIds::OPENR](
          AdminDistance::DIRECTLY_CONNECTED));
  entry->update(
      StdClientIds2ClientID(StdClientIds::BGPD),
      clientNextHopsEntry[StdClientIds::BGPD](
          AdminDistance::DIRECTLY_CONNECTED));

  entry->delEntryForClient(StdClientIds2ClientID(StdClientIds::OPENR));

  EXPECT_EQ(
      nullptr,
      entry->getEntryForClient(StdClientIds2ClientID(StdClientIds::OPENR)));
  EXPECT_EQ(
      getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED),
      *(entry->getEntryForClient(StdClientIds2ClientID(StdClientIds::BGPD))));
}

TEST(LabelForwardingEntryTests, getBestEntry) {
  std::map<StdClientIds, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {StdClientIds::OPENR, getSwapLabelNextHopEntry},
          {StdClientIds::BGPD, getPushLabelNextHopEntry},
          {StdClientIds::STATIC_ROUTE, getPhpLabelNextHopEntry},
          {StdClientIds::INTERFACE_ROUTE, getPopLabelNextHopEntry},
      };

  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      clientNextHopsEntry[StdClientIds::OPENR](
          AdminDistance::DIRECTLY_CONNECTED));

  for (auto stdClientId : {StdClientIds::BGPD,
                           StdClientIds::STATIC_ROUTE,
                           StdClientIds::INTERFACE_ROUTE}) {
    entry->update(
        StdClientIds2ClientID(stdClientId),
        clientNextHopsEntry[stdClientId](AdminDistance::DIRECTLY_CONNECTED));
  }

  EXPECT_EQ(
    clientNextHopsEntry[StdClientIds::OPENR](
        AdminDistance::DIRECTLY_CONNECTED), entry->getLabelNextHop()
      );

  entry->delEntryForClient(StdClientIds2ClientID(StdClientIds::OPENR));
  EXPECT_EQ(
      clientNextHopsEntry[StdClientIds::BGPD](
          AdminDistance::DIRECTLY_CONNECTED),
      entry->getLabelNextHop());
  entry->delEntryForClient(StdClientIds2ClientID(StdClientIds::BGPD));
  EXPECT_EQ(
      clientNextHopsEntry[StdClientIds::STATIC_ROUTE](
          AdminDistance::DIRECTLY_CONNECTED),
      entry->getLabelNextHop());
  entry->delEntryForClient(
      StdClientIds2ClientID(StdClientIds::STATIC_ROUTE));
  EXPECT_EQ(
      clientNextHopsEntry[StdClientIds::INTERFACE_ROUTE](
          AdminDistance::DIRECTLY_CONNECTED),
      entry->getLabelNextHop());
}
