// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/Utils.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/test/LabelForwardingUtils.h"

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
