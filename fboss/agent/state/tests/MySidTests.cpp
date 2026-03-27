// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "folly/IPAddressV6.h"

using namespace facebook::fboss;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(10)}};
}

folly::CIDRNetwork makeSidPrefix(
    const std::string& addr = "fc00:100::1",
    uint8_t len = 48) {
  return std::make_pair(folly::IPAddress(addr), len);
}

std::shared_ptr<MySid> makeMySid(
    const folly::CIDRNetwork& prefix = makeSidPrefix()) {
  state::MySidFields fields;
  fields.type() = MySidType::NODE_MICRO_SID;
  facebook::network::thrift::IPPrefix thriftPrefix;
  thriftPrefix.prefixAddress() =
      facebook::network::toBinaryAddress(prefix.first);
  thriftPrefix.prefixLength() = prefix.second;
  fields.mySid() = thriftPrefix;
  return std::make_shared<MySid>(fields);
}
} // namespace

TEST(MySidTest, GettersReturnSetValues) {
  auto mySid = makeMySid();
  EXPECT_EQ(mySid->getType(), MySidType::NODE_MICRO_SID);
  auto cidr = mySid->getMySid();
  EXPECT_EQ(cidr.first, folly::IPAddress("fc00:100::1"));
  EXPECT_EQ(cidr.second, 48);
}

TEST(MySidTest, GetID) {
  auto mySid = makeMySid();
  EXPECT_EQ(mySid->getID(), "fc00:100::1/48");
}

TEST(MySidTest, ModifyType) {
  auto mySid = makeMySid();
  mySid->setType(MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_EQ(mySid->getType(), MySidType::DECAPSULATE_AND_LOOKUP);
}

TEST(MySidTest, Inequality) {
  auto mySid0 = makeMySid(makeSidPrefix("fc00:100::1", 48));
  auto mySid1 = makeMySid(makeSidPrefix("fc00:200::1", 48));
  EXPECT_FALSE(*mySid0 == *mySid1);
}

TEST(MySidTest, SerDeserMySid) {
  auto mySid = makeMySid();
  auto serialized = mySid->toThrift();
  auto mySidBack = std::make_shared<MySid>(serialized);
  EXPECT_TRUE(*mySid == *mySidBack);
}

TEST(MySidTest, SerDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  auto mySid0 = makeMySid(makeSidPrefix("fc00:100::1", 48));
  auto mySid1 = makeMySid(makeSidPrefix("fc00:200::1", 64));

  auto mySids = state->getMySids()->modify(&state);
  mySids->addNode(mySid0, scope());
  mySids->addNode(mySid1, scope());

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);

  for (const auto& key : {"fc00:100::1/48", "fc00:200::1/64"}) {
    EXPECT_TRUE(
        *state->getMySids()->getNode(key) ==
        *stateBack->getMySids()->getNode(key));
  }
}

TEST(MySidTest, AddRemove) {
  auto state = std::make_shared<SwitchState>();

  auto mySid0 = makeMySid(makeSidPrefix("fc00:100::1", 48));
  auto mySid1 = makeMySid(makeSidPrefix("fc00:200::1", 64));

  auto mySids = state->getMySids()->modify(&state);
  mySids->addNode(mySid0, scope());
  mySids->addNode(mySid1, scope());
  mySids->removeNode("fc00:100::1/48");
  EXPECT_EQ(state->getMySids()->getNodeIf("fc00:100::1/48"), nullptr);
  EXPECT_NE(state->getMySids()->getNodeIf("fc00:200::1/64"), nullptr);
}

TEST(MySidDeltaTest, EmptyDelta) {
  auto oldState = std::make_shared<SwitchState>();
  oldState->publish();
  auto newState = std::make_shared<SwitchState>();
  newState->publish();
  StateDelta delta(oldState, newState);

  std::set<std::string> added, removed, changed;
  DeltaFunctions::forEachChanged(
      delta.getMySidsDelta(),
      [&](const auto& oldEntry, const auto& newEntry) {
        changed.insert(oldEntry->getID());
      },
      [&](const auto& entry) { added.insert(entry->getID()); },
      [&](const auto& entry) { removed.insert(entry->getID()); });

  EXPECT_TRUE(added.empty());
  EXPECT_TRUE(removed.empty());
  EXPECT_TRUE(changed.empty());
}

TEST(MySidDeltaTest, AddEntries) {
  auto oldState = std::make_shared<SwitchState>();
  oldState->publish();

  auto newState = oldState->clone();
  auto mySid0 = makeMySid(makeSidPrefix("fc00:100::1", 48));
  auto mySid1 = makeMySid(makeSidPrefix("fc00:200::1", 64));
  auto mySids = newState->getMySids()->modify(&newState);
  mySids->addNode(mySid0, scope());
  mySids->addNode(mySid1, scope());
  newState->publish();

  StateDelta delta(oldState, newState);

  std::set<std::string> added, removed, changed;
  DeltaFunctions::forEachChanged(
      delta.getMySidsDelta(),
      [&](const auto& oldEntry, const auto& newEntry) {
        changed.insert(oldEntry->getID());
      },
      [&](const auto& entry) { added.insert(entry->getID()); },
      [&](const auto& entry) { removed.insert(entry->getID()); });

  EXPECT_EQ(added, (std::set<std::string>{"fc00:100::1/48", "fc00:200::1/64"}));
  EXPECT_TRUE(removed.empty());
  EXPECT_TRUE(changed.empty());
}

TEST(MySidDeltaTest, RemoveEntries) {
  auto oldState = std::make_shared<SwitchState>();
  auto mySid0 = makeMySid(makeSidPrefix("fc00:100::1", 48));
  auto mySid1 = makeMySid(makeSidPrefix("fc00:200::1", 64));
  auto mySids = oldState->getMySids()->modify(&oldState);
  mySids->addNode(mySid0, scope());
  mySids->addNode(mySid1, scope());
  oldState->publish();

  auto newState = oldState->clone();
  auto newMySids = newState->getMySids()->modify(&newState);
  newMySids->removeNode("fc00:100::1/48");
  newMySids->removeNode("fc00:200::1/64");
  newState->publish();

  StateDelta delta(oldState, newState);

  std::set<std::string> added, removed, changed;
  DeltaFunctions::forEachChanged(
      delta.getMySidsDelta(),
      [&](const auto& oldEntry, const auto& newEntry) {
        changed.insert(oldEntry->getID());
      },
      [&](const auto& entry) { added.insert(entry->getID()); },
      [&](const auto& entry) { removed.insert(entry->getID()); });

  EXPECT_TRUE(added.empty());
  EXPECT_EQ(
      removed, (std::set<std::string>{"fc00:100::1/48", "fc00:200::1/64"}));
  EXPECT_TRUE(changed.empty());
}

TEST(MySidDeltaTest, ChangeEntry) {
  auto oldState = std::make_shared<SwitchState>();
  auto mySid = makeMySid(makeSidPrefix("fc00:100::1", 48));
  auto mySids = oldState->getMySids()->modify(&oldState);
  mySids->addNode(mySid, scope());
  oldState->publish();

  auto newState = oldState->clone();
  auto newMySids = newState->getMySids()->modify(&newState);
  auto updatedMySid = newMySids->getNode("fc00:100::1/48")->clone();
  updatedMySid->setType(MySidType::DECAPSULATE_AND_LOOKUP);
  newMySids->updateNode(updatedMySid, scope());
  newState->publish();

  StateDelta delta(oldState, newState);

  std::set<std::string> added, removed, changed;
  DeltaFunctions::forEachChanged(
      delta.getMySidsDelta(),
      [&](const auto& oldEntry, const auto& newEntry) {
        EXPECT_EQ(oldEntry->getType(), MySidType::NODE_MICRO_SID);
        EXPECT_EQ(newEntry->getType(), MySidType::DECAPSULATE_AND_LOOKUP);
        changed.insert(oldEntry->getID());
      },
      [&](const auto& entry) { added.insert(entry->getID()); },
      [&](const auto& entry) { removed.insert(entry->getID()); });

  EXPECT_TRUE(added.empty());
  EXPECT_TRUE(removed.empty());
  EXPECT_EQ(changed, (std::set<std::string>{"fc00:100::1/48"}));
}

TEST(MySidTest, ResolvedNextHop) {
  auto mySid = makeMySid();
  EXPECT_EQ(mySid->getResolvedNextHop(), nullptr);

  RouteNextHopEntry nhop(
      RouteNextHopEntry::Action::DROP, AdminDistance::STATIC_ROUTE);
  mySid->setResolvedNextHop(std::optional<RouteNextHopEntry>(nhop.toThrift()));
  auto* got = mySid->getResolvedNextHop();
  ASSERT_NE(got, nullptr);
  EXPECT_EQ(got->getAction(), RouteNextHopEntry::Action::DROP);
}

TEST(MySidTest, UnresolvedNextHop) {
  auto mySid = makeMySid();
  EXPECT_EQ(mySid->getUnresolvedNextHop(), nullptr);

  RouteNextHopEntry nhop(
      RouteNextHopEntry::Action::TO_CPU, AdminDistance::STATIC_ROUTE);
  mySid->setUnresolvedNextHop(
      std::optional<RouteNextHopEntry>(nhop.toThrift()));
  auto* got = mySid->getUnresolvedNextHop();
  ASSERT_NE(got, nullptr);
  EXPECT_EQ(got->getAction(), RouteNextHopEntry::Action::TO_CPU);
}

TEST(MySidTest, SerDeserWithNextHops) {
  auto mySid = makeMySid();
  RouteNextHopEntry resolved(
      RouteNextHopEntry::Action::DROP, AdminDistance::STATIC_ROUTE);
  RouteNextHopEntry unresolved(
      RouteNextHopEntry::Action::TO_CPU, AdminDistance::STATIC_ROUTE);
  mySid->setResolvedNextHop(
      std::optional<RouteNextHopEntry>(resolved.toThrift()));
  mySid->setUnresolvedNextHop(
      std::optional<RouteNextHopEntry>(unresolved.toThrift()));

  auto serialized = mySid->toThrift();
  auto mySidBack = std::make_shared<MySid>(serialized);
  EXPECT_TRUE(*mySid == *mySidBack);

  auto* gotResolved = mySidBack->getResolvedNextHop();
  ASSERT_NE(gotResolved, nullptr);
  EXPECT_EQ(gotResolved->getAction(), RouteNextHopEntry::Action::DROP);

  auto* gotUnresolved = mySidBack->getUnresolvedNextHop();
  ASSERT_NE(gotUnresolved, nullptr);
  EXPECT_EQ(gotUnresolved->getAction(), RouteNextHopEntry::Action::TO_CPU);
}

TEST(MySidDeltaTest, MixedAddRemoveChange) {
  auto oldState = std::make_shared<SwitchState>();
  auto mySid0 = makeMySid(makeSidPrefix("fc00:100::1", 48));
  auto mySid1 = makeMySid(makeSidPrefix("fc00:200::1", 64));
  auto mySids = oldState->getMySids()->modify(&oldState);
  mySids->addNode(mySid0, scope());
  mySids->addNode(mySid1, scope());
  oldState->publish();

  auto newState = oldState->clone();
  auto newMySids = newState->getMySids()->modify(&newState);
  // Remove mySid0
  newMySids->removeNode("fc00:100::1/48");
  // Change mySid1
  auto updatedMySid1 = newMySids->getNode("fc00:200::1/64")->clone();
  updatedMySid1->setType(MySidType::DECAPSULATE_AND_LOOKUP);
  newMySids->updateNode(updatedMySid1, scope());
  // Add mySid2
  auto mySid2 = makeMySid(makeSidPrefix("fc00:300::1", 128));
  newMySids->addNode(mySid2, scope());
  newState->publish();

  StateDelta delta(oldState, newState);

  std::set<std::string> added, removed, changed;
  DeltaFunctions::forEachChanged(
      delta.getMySidsDelta(),
      [&](const auto& oldEntry, const auto& newEntry) {
        changed.insert(oldEntry->getID());
      },
      [&](const auto& entry) { added.insert(entry->getID()); },
      [&](const auto& entry) { removed.insert(entry->getID()); });

  EXPECT_EQ(added, (std::set<std::string>{"fc00:300::1/128"}));
  EXPECT_EQ(removed, (std::set<std::string>{"fc00:100::1/48"}));
  EXPECT_EQ(changed, (std::set<std::string>{"fc00:200::1/64"}));
}
