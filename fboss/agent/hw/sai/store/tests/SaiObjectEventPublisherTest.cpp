/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/store/SaiObjectEventPublisher.h"
#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

/*
 * A test subscriber that tracks notifications from a SaiNeighborTraits
 * publisher. Used to verify the pub/sub framework mechanics.
 */
class TestNeighborSubscriber
    : public detail::SaiObjectEventSubscriber<SaiNeighborTraits> {
 public:
  using Base = detail::SaiObjectEventSubscriber<SaiNeighborTraits>;

  explicit TestNeighborSubscriber(
      typename PublisherKey<SaiNeighborTraits>::type key)
      : Base(key) {}

  void afterCreate(PublisherObjectSharedPtr object) override {
    createCount_++;
    setPublisherObject(object);
  }

  void beforeRemove() override {
    removeCount_++;
    setPublisherObject(nullptr);
  }

  void linkDown() override {
    linkDownCount_++;
  }

  int createCount_{0};
  int removeCount_{0};
  int linkDownCount_{0};
};

/*
 * A test aggregate subscriber with two publishers (SaiNeighborTraits +
 * SaiIpNextHopTraits). Used to verify the aggregate subscriber pattern
 * where createObject is called only when ALL publishers are alive.
 */
class TestDualPubAggregateSubscriber : public SaiObjectEventAggregateSubscriber<
                                           TestDualPubAggregateSubscriber,
                                           SaiNextHopGroupMemberTraits,
                                           SaiNeighborTraits,
                                           SaiIpNextHopTraits> {
 public:
  using Base = SaiObjectEventAggregateSubscriber<
      TestDualPubAggregateSubscriber,
      SaiNextHopGroupMemberTraits,
      SaiNeighborTraits,
      SaiIpNextHopTraits>;
  using PublishedObjects = std::tuple<
      std::weak_ptr<const SaiObject<SaiNeighborTraits>>,
      std::weak_ptr<const SaiObject<SaiIpNextHopTraits>>>;

  TestDualPubAggregateSubscriber(
      typename PublisherKey<SaiNeighborTraits>::type neighborKey,
      typename PublisherKey<SaiIpNextHopTraits>::type nextHopKey)
      : Base(neighborKey, nextHopKey) {}

  void createObject(PublishedObjects /*added*/) {
    createCount_++;
  }

  void removeObject(size_t index, PublishedObjects /*removed*/) {
    removeCount_++;
    lastRemovedIndex_ = index;
  }

  void handleLinkDown() {
    linkDownCount_++;
  }

  int createCount_{0};
  int removeCount_{0};
  int linkDownCount_{0};
  size_t lastRemovedIndex_{0};
};

class SaiObjectEventPublisherTest : public SaiStoreTest {
 public:
  SaiNeighborTraits::NeighborEntry makeNeighborEntry(
      const std::string& ip) const {
    return SaiNeighborTraits::NeighborEntry(0, 0, folly::IPAddress(ip));
  }

  SaiNeighborTraits::CreateAttributes makeNeighborAttrs() const {
    folly::MacAddress dstMac{"42:42:42:42:42:42"};
    return {dstMac, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
  }

  SaiIpNextHopTraits::AdapterHostKey makeNextHopKey(
      const std::string& ip) const {
    return SaiIpNextHopTraits::AdapterHostKey{42, folly::IPAddress(ip)};
  }

  SaiIpNextHopTraits::CreateAttributes makeNextHopAttrs(
      const std::string& ip) const {
    return {SAI_NEXT_HOP_TYPE_IP, 42, folly::IPAddress(ip), std::nullopt};
  }

  detail::SaiObjectEventPublisher<SaiNeighborTraits>& neighborPublisher() {
    return SaiObjectEventPublisher::getInstance()->get<SaiNeighborTraits>();
  }

  detail::SaiObjectEventPublisher<SaiIpNextHopTraits>& nextHopPublisher() {
    return SaiObjectEventPublisher::getInstance()->get<SaiIpNextHopTraits>();
  }
};

TEST_F(SaiObjectEventPublisherTest, subscriberReceivesCreateNotification) {
  auto entry = makeNeighborEntry("10.0.0.1");
  auto subscriber = std::make_shared<TestNeighborSubscriber>(entry);
  neighborPublisher().subscribe(subscriber);

  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());

  EXPECT_EQ(subscriber->createCount_, 1);
  EXPECT_TRUE(subscriber->isReady());
}

TEST_F(SaiObjectEventPublisherTest, subscriberReceivesDeleteNotification) {
  auto entry = makeNeighborEntry("10.0.0.2");
  auto subscriber = std::make_shared<TestNeighborSubscriber>(entry);
  neighborPublisher().subscribe(subscriber);

  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());
  EXPECT_EQ(subscriber->createCount_, 1);

  // Dropping the only shared_ptr destroys the object, triggering
  // notifyBeforeDestroy
  obj.reset();

  EXPECT_EQ(subscriber->removeCount_, 1);
  EXPECT_FALSE(subscriber->isReady());
}

TEST_F(SaiObjectEventPublisherTest, subscriberGetsCorrectPublisherObject) {
  auto entry = makeNeighborEntry("10.0.0.3");
  auto subscriber = std::make_shared<TestNeighborSubscriber>(entry);
  neighborPublisher().subscribe(subscriber);

  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());

  auto publisherObj = subscriber->getPublisherObject().lock();
  EXPECT_NE(publisherObj, nullptr);
  EXPECT_EQ(publisherObj->adapterKey(), entry);
}

TEST_F(SaiObjectEventPublisherTest, lateSubscriberImmediatelyNotified) {
  auto entry = makeNeighborEntry("10.0.0.4");

  // Create publisher object first
  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());

  // Subscribe after publisher is already live
  auto subscriber = std::make_shared<TestNeighborSubscriber>(entry);
  neighborPublisher().subscribe(subscriber);

  // Subscriber should be immediately notified since publisher is live
  EXPECT_EQ(subscriber->createCount_, 1);
  EXPECT_TRUE(subscriber->isReady());
}

TEST_F(SaiObjectEventPublisherTest, destroyedSubscriberNotNotified) {
  auto entry = makeNeighborEntry("10.0.0.5");

  auto& store = saiStore->get<SaiNeighborTraits>();

  {
    auto subscriber = std::make_shared<TestNeighborSubscriber>(entry);
    neighborPublisher().subscribe(subscriber);
    // subscriber goes out of scope here
  }

  // Creating a publisher object after subscriber is destroyed should not crash
  // (boost::signals2 track_foreign handles cleanup)
  auto obj = store.setObject(entry, makeNeighborAttrs());
}

TEST_F(SaiObjectEventPublisherTest, multipleSubscribersAllNotified) {
  auto entry = makeNeighborEntry("10.0.0.6");
  auto sub1 = std::make_shared<TestNeighborSubscriber>(entry);
  auto sub2 = std::make_shared<TestNeighborSubscriber>(entry);
  neighborPublisher().subscribe(sub1);
  neighborPublisher().subscribe(sub2);

  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());

  EXPECT_EQ(sub1->createCount_, 1);
  EXPECT_EQ(sub2->createCount_, 1);

  obj.reset();

  EXPECT_EQ(sub1->removeCount_, 1);
  EXPECT_EQ(sub2->removeCount_, 1);
}

TEST_F(SaiObjectEventPublisherTest, subscriberReceivesLinkDownNotification) {
  auto entry = makeNeighborEntry("10.0.0.7");
  auto subscriber = std::make_shared<TestNeighborSubscriber>(entry);
  neighborPublisher().subscribe(subscriber);

  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());
  EXPECT_EQ(subscriber->createCount_, 1);

  neighborPublisher().notifyLinkDown(entry);

  EXPECT_EQ(subscriber->linkDownCount_, 1);
  // linkDown should not trigger create or remove
  EXPECT_EQ(subscriber->createCount_, 1);
  EXPECT_EQ(subscriber->removeCount_, 0);
}

TEST_F(SaiObjectEventPublisherTest, linkDownPublisherStillLive) {
  auto entry = makeNeighborEntry("10.0.0.8");
  auto subscriber = std::make_shared<TestNeighborSubscriber>(entry);
  neighborPublisher().subscribe(subscriber);

  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());

  neighborPublisher().notifyLinkDown(entry);

  // Publisher object should still be considered live after linkDown
  EXPECT_TRUE(subscriber->isReady());
  auto publisherObj = subscriber->getPublisherObject().lock();
  EXPECT_NE(publisherObj, nullptr);
}

TEST_F(SaiObjectEventPublisherTest, linkDownWithNoSubscribers) {
  auto entry = makeNeighborEntry("10.0.0.9");

  auto& store = saiStore->get<SaiNeighborTraits>();
  auto obj = store.setObject(entry, makeNeighborAttrs());

  // Should not crash when no subscribers are listening
  neighborPublisher().notifyLinkDown(entry);
}

TEST_F(SaiObjectEventPublisherTest, aggregateNotCreatedWithPartialPublishers) {
  auto neighborEntry = makeNeighborEntry("10.0.0.10");
  auto nextHopKey = makeNextHopKey("10.0.0.10");
  auto sub = std::make_shared<TestDualPubAggregateSubscriber>(
      neighborEntry, nextHopKey);
  neighborPublisher().subscribe(sub);
  nextHopPublisher().subscribe(sub);

  // Only create neighbor (publisher 1)
  auto& neighborStore = saiStore->get<SaiNeighborTraits>();
  auto neighborObj =
      neighborStore.setObject(neighborEntry, makeNeighborAttrs());

  // createObject should NOT be called - only one of two publishers is alive
  EXPECT_EQ(sub->createCount_, 0);
  EXPECT_FALSE(sub->allPublishedObjectsAlive());
}

TEST_F(SaiObjectEventPublisherTest, aggregateCreatedWhenAllPublishersAlive) {
  auto neighborEntry = makeNeighborEntry("10.0.0.11");
  auto nextHopKey = makeNextHopKey("10.0.0.11");
  auto sub = std::make_shared<TestDualPubAggregateSubscriber>(
      neighborEntry, nextHopKey);
  neighborPublisher().subscribe(sub);
  nextHopPublisher().subscribe(sub);

  // Create first publisher
  auto& neighborStore = saiStore->get<SaiNeighborTraits>();
  auto neighborObj =
      neighborStore.setObject(neighborEntry, makeNeighborAttrs());
  EXPECT_EQ(sub->createCount_, 0);

  // Create second publisher - now all are alive
  auto& nextHopStore = saiStore->get<SaiIpNextHopTraits>();
  auto nextHopObj =
      nextHopStore.setObject(nextHopKey, makeNextHopAttrs("10.0.0.11"));

  EXPECT_EQ(sub->createCount_, 1);
  EXPECT_TRUE(sub->allPublishedObjectsAlive());
}

TEST_F(SaiObjectEventPublisherTest, aggregateRemoveOnPublisherDeath) {
  auto neighborEntry = makeNeighborEntry("10.0.0.12");
  auto nextHopKey = makeNextHopKey("10.0.0.12");
  auto sub = std::make_shared<TestDualPubAggregateSubscriber>(
      neighborEntry, nextHopKey);
  neighborPublisher().subscribe(sub);
  nextHopPublisher().subscribe(sub);

  auto& neighborStore = saiStore->get<SaiNeighborTraits>();
  auto neighborObj =
      neighborStore.setObject(neighborEntry, makeNeighborAttrs());
  auto& nextHopStore = saiStore->get<SaiIpNextHopTraits>();
  auto nextHopObj =
      nextHopStore.setObject(nextHopKey, makeNextHopAttrs("10.0.0.12"));
  EXPECT_EQ(sub->createCount_, 1);

  // Remove neighbor (index 0 in AggregateType tuple)
  neighborObj.reset();

  EXPECT_EQ(sub->removeCount_, 1);
  EXPECT_EQ(sub->lastRemovedIndex_, 0);
  EXPECT_FALSE(sub->allPublishedObjectsAlive());
}

TEST_F(SaiObjectEventPublisherTest, aggregateLinkDown) {
  auto neighborEntry = makeNeighborEntry("10.0.0.13");
  auto nextHopKey = makeNextHopKey("10.0.0.13");
  auto sub = std::make_shared<TestDualPubAggregateSubscriber>(
      neighborEntry, nextHopKey);
  neighborPublisher().subscribe(sub);
  nextHopPublisher().subscribe(sub);

  auto& neighborStore = saiStore->get<SaiNeighborTraits>();
  auto neighborObj =
      neighborStore.setObject(neighborEntry, makeNeighborAttrs());
  auto& nextHopStore = saiStore->get<SaiIpNextHopTraits>();
  auto nextHopObj =
      nextHopStore.setObject(nextHopKey, makeNextHopAttrs("10.0.0.13"));

  neighborPublisher().notifyLinkDown(neighborEntry);

  EXPECT_EQ(sub->linkDownCount_, 1);
  // Publisher should still be alive after linkDown
  EXPECT_TRUE(sub->allPublishedObjectsAlive());
}
