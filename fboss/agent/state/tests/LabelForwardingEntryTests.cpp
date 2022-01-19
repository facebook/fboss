// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/Utils.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/LabelForwardingUtils.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss;

namespace {
void testToAndFromDynamic(const std::shared_ptr<LabelForwardingEntry>& entry) {
  EXPECT_TRUE(LabelForwardingEntry::fromFollyDynamic(entry->toFollyDynamic())
                  ->isSame(entry.get()));
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

TEST(LabelForwardingEntryTests, fromJsonOldFormat) {
  std::string jsonOldEntryType = R"(
  {
  "labelNextHop": {
    "action": "Nexthops",
    "adminDistance": 10,
    "nexthops": [
      {
        "interface": 2002,
        "label_forwarding_action": {
          "type": 2
        },
        "nexthop": "fe80::d8c4:97ff:fed0:5b14",
        "weight": "0"
      },
      {
        "interface": 2067,
        "label_forwarding_action": {
          "type": 2
        },
        "nexthop": "fe80::d8c4:97ff:fed0:5b14",
        "weight": "0"
      },
      {
        "interface": 2095,
        "label_forwarding_action": {
          "type": 2
        },
        "nexthop": "fe80::d8c4:97ff:fed0:5b14",
        "weight": "0"
      }
    ]
  },
  "labelNextHopMulti": {
    "786": {
      "action": "Nexthops",
      "adminDistance": 10,
      "nexthops": [
        {
          "interface": 2002,
          "label_forwarding_action": {
            "type": 2
          },
          "nexthop": "fe80::d8c4:97ff:fed0:5b14",
          "weight": "0"
        },
        {
          "interface": 2067,
          "label_forwarding_action": {
            "type": 2
          },
          "nexthop": "fe80::d8c4:97ff:fed0:5b14",
          "weight": "0"
        },
        {
          "interface": 2095,
          "label_forwarding_action": {
            "type": 2
          },
          "nexthop": "fe80::d8c4:97ff:fed0:5b14",
          "weight": "0"
        }
      ]
    }
  },
  "topLabel": 1001
  })";
  auto entry = LabelForwardingInformationBase::labelEntryFromFollyDynamic(
      folly::parseJson(jsonOldEntryType));
  auto oldFormatJsonWritten =
      LabelForwardingInformationBase::toFollyDynamicOldFormat(entry);
  EXPECT_EQ(folly::parseJson(jsonOldEntryType), oldFormatJsonWritten);
  std::string jsonNewEntryType = R"(
  {
  "forwardingInfo": {
    "adminDistance": 10,
    "nexthops": [
      {
        "interface": 2002,
        "label_forwarding_action": {
          "type": 2
        },
        "nexthop": "fe80::d8c4:97ff:fed0:5b14",
        "weight": "0"
      },
      {
        "interface": 2067,
        "label_forwarding_action": {
          "type": 2
        },
        "nexthop": "fe80::d8c4:97ff:fed0:5b14",
        "weight": "0"
      },
      {
        "label_forwarding_action": {
          "type": 2
        },
        "interface": 2095,
        "nexthop": "fe80::d8c4:97ff:fed0:5b14",
        "weight": "0"
      }
    ],
    "action": "Nexthops"
  },
  "flags": 2,
  "prefix": {
    "label": 1001
  },
  "rib": {
    "786": {
      "adminDistance": 10,
      "action": "Nexthops",
      "nexthops": [
        {
          "interface": 2002,
          "label_forwarding_action": {
            "type": 2
          },
          "weight": "0",
          "nexthop": "fe80::d8c4:97ff:fed0:5b14"
        },
        {
          "label_forwarding_action": {
            "type": 2
          },
          "interface": 2067,
          "nexthop": "fe80::d8c4:97ff:fed0:5b14",
          "weight": "0"
        },
        {
          "label_forwarding_action": {
            "type": 2
          },
          "interface": 2095,
          "weight": "0",
          "nexthop": "fe80::d8c4:97ff:fed0:5b14"
        }
      ]
    }
  }
})";
  auto newEntry = LabelForwardingInformationBase::labelEntryFromFollyDynamic(
      folly::parseJson(jsonNewEntryType));
  EXPECT_TRUE(newEntry->isSame(entry.get()));
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
  LabelForwardingInformationBase::resolve(entry);

  EXPECT_EQ(
      clientNextHopsEntry[ClientID::OPENR](AdminDistance::DIRECTLY_CONNECTED),
      entry->getForwardInfo());

  entry->delEntryForClient(ClientID::OPENR);
  LabelForwardingInformationBase::resolve(entry);
  EXPECT_EQ(
      clientNextHopsEntry[ClientID::BGPD](AdminDistance::DIRECTLY_CONNECTED),
      entry->getForwardInfo());
  entry->delEntryForClient(ClientID::BGPD);
  LabelForwardingInformationBase::resolve(entry);
  EXPECT_EQ(
      clientNextHopsEntry[ClientID::STATIC_ROUTE](
          AdminDistance::DIRECTLY_CONNECTED),
      entry->getForwardInfo());
  entry->delEntryForClient(ClientID::STATIC_ROUTE);
  LabelForwardingInformationBase::resolve(entry);
  EXPECT_EQ(
      clientNextHopsEntry[ClientID::INTERFACE_ROUTE](
          AdminDistance::DIRECTLY_CONNECTED),
      entry->getForwardInfo());
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
  auto* modifiedEntry =
      publishedLfib->modifyLabelEntry(&newerState, publishedEntry);
  EXPECT_NE(newerState.get(), newState.get());
  EXPECT_NE(
      newerState->getLabelForwardingInformationBase().get(),
      publishedLfib.get());
  EXPECT_NE(modifiedEntry, publishedEntry.get());

  auto modifiedLfib = newerState->getLabelForwardingInformationBase();
  EXPECT_EQ(
      modifiedEntry,
      modifiedLfib->modifyLabelEntry(
          &newerState, modifiedLfib->getLabelForwardingEntryIf(5001)));
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
  LabelForwardingInformationBase::resolve(entry);
  const auto& nexthop = entry->getForwardInfo();
  EXPECT_NE(nexthop.getNextHopSet().size(), 0);
}
