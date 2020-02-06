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
          ClientID::OPENR,
          util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          ClientID::OPENR,
          util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          ClientID::OPENR,
          util::getPhpLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)),
      std::make_shared<LabelForwardingEntry>(
          5001,
          ClientID::OPENR,
          util::getPopLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED))};
  for (const auto& entry : entries) {
    testToAndFromDynamic(entry);
  }
}

TEST(LabelForwardingEntryTests, getEntryForClient) {
  std::map<ClientID, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {ClientID::OPENR,
           [](AdminDistance distance) {
             return util::getSwapLabelNextHopEntry(distance);
           }},
          {ClientID::BGPD,
           [](AdminDistance distance) {
             return util::getPushLabelNextHopEntry(distance);
           }},
          {ClientID::STATIC_ROUTE,
           [](AdminDistance distance) {
             return util::getPhpLabelNextHopEntry(distance);
           }},
          {ClientID::INTERFACE_ROUTE,
           [](AdminDistance distance) {
             return util::getPopLabelNextHopEntry(distance);
           }},
      };

  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      clientNextHopsEntry[ClientID::OPENR](AdminDistance::DIRECTLY_CONNECTED));

  for (auto clientID :
       {ClientID::BGPD, ClientID::STATIC_ROUTE, ClientID::INTERFACE_ROUTE}) {
    entry->update(
        clientID,
        clientNextHopsEntry[clientID](AdminDistance::DIRECTLY_CONNECTED));
  }

  for (const auto clientNextHop : clientNextHopsEntry) {
    auto* nexthopEntry = entry->getEntryForClient(clientNextHop.first);
    EXPECT_EQ(
        *nexthopEntry, clientNextHop.second(AdminDistance::DIRECTLY_CONNECTED));
  }
}

TEST(LabelForwardingEntryTests, delEntryForClient) {
  std::map<ClientID, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {ClientID::OPENR,
           [](AdminDistance distance) {
             return util::getSwapLabelNextHopEntry(distance);
           }},
          {ClientID::BGPD,
           [](AdminDistance distance) {
             return util::getPushLabelNextHopEntry(distance);
           }},
      };

  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      clientNextHopsEntry[ClientID::OPENR](AdminDistance::DIRECTLY_CONNECTED));
  entry->update(
      ClientID::BGPD,
      clientNextHopsEntry[ClientID::BGPD](AdminDistance::DIRECTLY_CONNECTED));

  entry->delEntryForClient(ClientID::OPENR);

  EXPECT_EQ(nullptr, entry->getEntryForClient(ClientID::OPENR));
  EXPECT_EQ(
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED),
      *(entry->getEntryForClient(ClientID::BGPD)));
}

TEST(LabelForwardingEntryTests, getBestEntry) {
  std::map<ClientID, std::function<LabelNextHopEntry(AdminDistance)>>
      clientNextHopsEntry{
          {ClientID::OPENR,
           [](AdminDistance distance) {
             return util::getSwapLabelNextHopEntry(distance);
           }},
          {ClientID::BGPD,
           [](AdminDistance distance) {
             return util::getPushLabelNextHopEntry(distance);
           }},
          {ClientID::STATIC_ROUTE,
           [](AdminDistance distance) {
             return util::getPhpLabelNextHopEntry(distance);
           }},
          {ClientID::INTERFACE_ROUTE,
           [](AdminDistance distance) {
             return util::getPopLabelNextHopEntry(distance);
           }},
      };

  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      clientNextHopsEntry[ClientID::OPENR](AdminDistance::DIRECTLY_CONNECTED));

  for (auto clientID :
       {ClientID::BGPD, ClientID::STATIC_ROUTE, ClientID::INTERFACE_ROUTE}) {
    entry->update(
        clientID,
        clientNextHopsEntry[clientID](AdminDistance::DIRECTLY_CONNECTED));
  }

  EXPECT_EQ(
      clientNextHopsEntry[ClientID::OPENR](AdminDistance::DIRECTLY_CONNECTED),
      entry->getLabelNextHop());

  entry->delEntryForClient(ClientID::OPENR);
  EXPECT_EQ(
      clientNextHopsEntry[ClientID::BGPD](AdminDistance::DIRECTLY_CONNECTED),
      entry->getLabelNextHop());
  entry->delEntryForClient(ClientID::BGPD);
  EXPECT_EQ(
      clientNextHopsEntry[ClientID::STATIC_ROUTE](
          AdminDistance::DIRECTLY_CONNECTED),
      entry->getLabelNextHop());
  entry->delEntryForClient(ClientID::STATIC_ROUTE);
  EXPECT_EQ(
      clientNextHopsEntry[ClientID::INTERFACE_ROUTE](
          AdminDistance::DIRECTLY_CONNECTED),
      entry->getLabelNextHop());
}

TEST(LabelForwardingEntryTests, modify) {
  auto oldState = testStateA();
  auto lFib = std::make_shared<LabelForwardingInformationBase>();
  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
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
  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  const auto& nexthop = entry->getLabelNextHop();
  EXPECT_NE(nexthop.getNextHopSet().size(), 0);
}
