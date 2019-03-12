// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/Utils.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/LabelForwardingUtils.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss;

namespace {
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
          util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          util::getPhpLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          util::getPopLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED))};
  for (const auto& entry : entries) {
    testToAndFromDynamic(entry);
  }
}

TEST(LabelForwardingEntryTests, getEntryForClient) {
  std::map<StdClientIds, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {StdClientIds::OPENR, util::getSwapLabelNextHopEntry},
          {StdClientIds::BGPD, util::getPushLabelNextHopEntry},
          {StdClientIds::STATIC_ROUTE, util::getPhpLabelNextHopEntry},
          {StdClientIds::INTERFACE_ROUTE, util::getPopLabelNextHopEntry},
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
          {StdClientIds::OPENR, util::getSwapLabelNextHopEntry},
          {StdClientIds::BGPD, util::getPushLabelNextHopEntry},
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
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED),
      *(entry->getEntryForClient(StdClientIds2ClientID(StdClientIds::BGPD))));
}

TEST(LabelForwardingEntryTests, getBestEntry) {
  std::map<StdClientIds, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {StdClientIds::OPENR, util::getSwapLabelNextHopEntry},
          {StdClientIds::BGPD, util::getPushLabelNextHopEntry},
          {StdClientIds::STATIC_ROUTE, util::getPhpLabelNextHopEntry},
          {StdClientIds::INTERFACE_ROUTE, util::getPopLabelNextHopEntry},
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

TEST(LabelForwardingEntryTests, modify) {
  auto oldState = testStateA();
  auto lFib = std::make_shared<LabelForwardingInformationBase>();
  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)));

  auto newState = oldState->clone();
  newState->resetLabelForwardingInformationBase(lFib);
  newState->publish();

  auto publishedLfib = newState->getLabelForwardingInformationBase();
  auto publishedEntry = publishedLfib->getLabelForwardingEntryIf(5001);
  ASSERT_NE(nullptr, publishedEntry);
  EXPECT_EQ(true, publishedLfib->isPublished());
  EXPECT_EQ(true, publishedEntry->isPublished());

  auto newerState = newState->clone();
  auto* modifiedEntry = publishedEntry->modify(&newerState);
  EXPECT_NE(newerState.get(), newState.get());
  EXPECT_NE(
      newerState->getLabelForwardingInformationBase().get(),
      publishedLfib.get());
  EXPECT_NE(modifiedEntry, publishedEntry.get());

  EXPECT_EQ(modifiedEntry, modifiedEntry->modify(&newerState));
  EXPECT_EQ(false, newerState->isPublished());
  EXPECT_EQ(false, modifiedEntry->isPublished());
  EXPECT_EQ(
      false, newerState->getLabelForwardingInformationBase()->isPublished());
  EXPECT_EQ(
      false,
      newerState->getLabelForwardingInformationBase()
          ->getLabelForwardingEntryIf(5001)
          ->isPublished());
}

TEST(LabelForwardingEntryTests, HasLabelNextHop) {
  auto entry =
      std::make_shared<LabelForwardingEntry>(
          5001,
          StdClientIds2ClientID(StdClientIds::OPENR),
          util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  const auto& nexthop = entry->getLabelNextHop();
  EXPECT_NE(nexthop.getNextHopSet().size(), 0);
}
