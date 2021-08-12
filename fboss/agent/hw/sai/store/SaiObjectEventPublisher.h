// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/signals2.hpp>

#include "fboss/agent/hw/sai/api/BridgeApi.h"
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"
#include "fboss/agent/hw/sai/store/Traits.h"

#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

template <>
struct IsPublisherKeyAdapterHostKey<SaiIpNextHopTraits> : std::true_type {};

template <>
struct IsPublisherKeyAdapterHostKey<SaiMplsNextHopTraits> : std::true_type {};

template <>
struct IsPublisherKeyAdapterHostKey<SaiNeighborTraits> : std::true_type {};

template <>
struct IsObjectPublisher<SaiIpNextHopTraits> : std::true_type {};

template <>
struct IsObjectPublisher<SaiMplsNextHopTraits> : std::true_type {};

template <>
struct IsObjectPublisher<SaiNeighborTraits> : std::true_type {};

template <>
struct IsObjectPublisher<SaiBridgePortTraits> : std::true_type {};

template <>
struct IsPublisherKeyCustomType<SaiBridgePortTraits> : std::true_type {};

template <>
struct PublisherKey<SaiBridgePortTraits>
    : detail::PublisherKeyInternal<SaiBridgePortTraits, SaiPortDescriptor> {};

template <>
struct IsObjectPublisher<SaiVlanRouterInterfaceTraits> : std::true_type {};

template <>
struct IsPublisherKeyCustomType<SaiVlanRouterInterfaceTraits> : std::true_type {
};

template <>
struct PublisherKey<SaiVlanRouterInterfaceTraits>
    : detail::PublisherKeyInternal<SaiVlanRouterInterfaceTraits, InterfaceID> {
};

template <>
struct IsObjectPublisher<SaiFdbTraits> : std::true_type {};

template <>
struct IsPublisherKeyCustomType<SaiFdbTraits> : std::true_type {};

template <>
struct PublisherKey<SaiFdbTraits>
    : detail::PublisherKeyInternal<
          SaiFdbTraits,
          std::tuple<InterfaceID, folly::MacAddress>> {};

template <typename>
class SaiObject;

namespace detail {
/* publisher mechanism
 * 1) provide interface to publishers to notify subscribers
 * 2) identifies publisher object by publisher attributes
 * 3) provide method to subscribers to subscribe to publisher
 * 4) issues self managed subscription, no tracking of publisher object if no
 * subscribers
 * 5) tracks live publishers, this is done to handle situation if
 * subscribers come after publishers without having subscribers to actively poll
 * publisher */
template <typename PublishedObjectTrait>
class SaiObjectEventPublisher {
 public:
  using Key = typename PublisherKey<PublishedObjectTrait>::type;
  using AdapterHostKey = typename PublishedObjectTrait::AdapterHostKey;
  using Subscriber = SaiObjectEventSubscriber<PublishedObjectTrait>;
  using PublisherObject = const SaiObject<PublishedObjectTrait>;

 private:
  class Subscription {
    // a subscription, using boost signals since they are fast in issuing
    // notifications to large number of subscribers, thread safe, re-entrant.
    template <typename T>
    using CreateSignal =
        boost::signals2::signal<void(std::shared_ptr<const T>)>;
    CreateSignal<SaiObject<PublishedObjectTrait>> createSignal_;
    using CreateSignalSlot =
        typename CreateSignal<SaiObject<PublishedObjectTrait>>::slot_type;

    using RemoveSignal = boost::signals2::signal<void()>;
    RemoveSignal removeSignal_;
    using RemoveSignalSlot = typename RemoveSignal::slot_type;

    using LinkDownSignal = boost::signals2::signal<void()>;
    LinkDownSignal linkDownSignal_;
    using LinkDownSignalSlot = typename LinkDownSignal::slot_type;

    friend class SaiObjectEventPublisher<PublishedObjectTrait>;
  };

 public:
  void subscribe(std::weak_ptr<Subscriber> subscriberWeakPtr) {
    auto subscriber = subscriberWeakPtr.lock(); // non-owning reference
    CHECK(subscriber);
    auto result = subscriptions_.refOrEmplace(subscriber->getPublisherKey());

    auto subscription = result.first;

    // add a subscriber here for create or remove notifications.
    // subscriptions are self managed, because they're put in ref map.
    // further if subscriber gets removed, its not notified because
    // "track_foreign" will delete its slot in general following principles hold
    // 1. a subscription exists only if at least one subscriber exists
    // 2. a subscription is deleted if no subscriber exists
    // 3. a slot for subscriber in subscription is freed if subscriber is
    // removed.
    // 4. a subscriber is not notified only if it exists
    typename Subscription::CreateSignalSlot createSignalSlot = std::bind(
        &Subscriber::afterCreate, subscriber.get(), std::placeholders::_1);
    subscription->createSignal_.connect(
        createSignalSlot.track_foreign(subscriberWeakPtr));

    typename Subscription::RemoveSignalSlot removeSignalSlot =
        std::bind(&Subscriber::beforeRemove, subscriber.get());
    subscription->removeSignal_.connect(
        removeSignalSlot.track_foreign(subscriberWeakPtr));

    typename Subscription::LinkDownSignalSlot linkDownSignalSlot =
        std::bind(&Subscriber::linkDown, subscriber.get());
    subscription->linkDownSignal_.connect(
        linkDownSignalSlot.track_foreign(subscriberWeakPtr));

    subscriber->saveSubscription(subscription);
    XLOGF(
        DBG3,
        "subscription added for publisher {}",
        subscriber->getPublisherKey());
    // check if publisher is already live
    auto publisher = livePublishers_.find(subscriber->getPublisherKey());
    if (publisher != livePublishers_.end()) {
      // notify only the subscriber who is subscribing
      subscriber->afterCreate(publisher->second.lock());
    }
  }

  void notifyCreate(Key key, const std::shared_ptr<PublisherObject> object) {
    livePublishers_.emplace(key, object);
    auto subscription = subscriptions_.get(key);
    if (!subscription) {
      return;
    }
    XLOGF(DBG3, "publisher object {} notify create", key);
    subscription->createSignal_(object);
  }

  void notifyDelete(Key key) {
    XLOGF(DBG3, "publisher object {} notify remove", key);
    livePublishers_.erase(key);
    auto subscription = subscriptions_.get(key);
    if (!subscription) {
      return;
    }
    subscription->removeSignal_();
  }

  void notifyLinkDown(Key key) {
    XLOGF(DBG3, "publisher object {} notify link down", key);
    auto subscription = subscriptions_.get(key);
    if (!subscription) {
      return;
    }
    subscription->linkDownSignal_();
  }

 private:
  std::unordered_map<Key, std::weak_ptr<PublisherObject>> livePublishers_;
  UnorderedRefMap<Key, Subscription> subscriptions_;
};

} // namespace detail

/* A singleton for easy access to publishing mechanism */
class SaiObjectEventPublisher {
 public:
  static std::shared_ptr<SaiObjectEventPublisher> getInstance();

  template <typename PublishedObjectTrait>
  void subscribe(
      std::shared_ptr<detail::SaiObjectEventSubscriber<PublishedObjectTrait>>
          subscriber) {
    std::get<PublishedObjectTrait>(publishers_).subscribe(subscriber);
  }

  template <typename PublishedObjectTrait>
  void notifyCreate(
      typename PublisherKey<PublishedObjectTrait>::type key,
      const std::shared_ptr<SaiObject<PublishedObjectTrait>> object) {
    std::get<PublishedObjectTrait>(publishers_).notifyCreate(key, object);
  }

  template <typename PublishedObjectTrait>
  void notifyDelete(typename PublisherKey<PublishedObjectTrait>::type key) {
    std::get<PublishedObjectTrait>(publishers_).notifyDelete(key);
  }

  template <typename PublishedObjectTrait>
  detail::SaiObjectEventPublisher<PublishedObjectTrait>& get() {
    return std::get<detail::SaiObjectEventPublisher<PublishedObjectTrait>>(
        publishers_);
  }

 private:
  std::tuple<
      detail::SaiObjectEventPublisher<SaiBridgePortTraits>,
      detail::SaiObjectEventPublisher<SaiFdbTraits>,
      detail::SaiObjectEventPublisher<SaiNeighborTraits>,
      detail::SaiObjectEventPublisher<SaiIpNextHopTraits>,
      detail::SaiObjectEventPublisher<SaiMplsNextHopTraits>,
      detail::SaiObjectEventPublisher<SaiVlanRouterInterfaceTraits>>
      publishers_;
};

} // namespace facebook::fboss
