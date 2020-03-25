// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/signals2.hpp>

#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

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
  using Key = typename PublishedObjectTrait::PublisherAttributes;
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

    friend class SaiObjectEventPublisher<PublishedObjectTrait>;
  };

 public:
  void subscribe(std::weak_ptr<Subscriber> subscriberWeakPtr) {
    auto subscriber = subscriberWeakPtr.lock(); // non-owning reference
    CHECK(subscriber);
    auto result =
        subscriptions_.refOrEmplace(subscriber->getPublisherAttributes());

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
    subscriber->saveSubscription(subscription);
  }

  void notifyCreate(Key key, const std::shared_ptr<PublisherObject> object) {
    livePublishers_.emplace(key, object);
    auto subscription = subscriptions_.get(key);
    if (!subscription) {
      return;
    }
    subscription->createSignal_(object);
  }

  void notifyDelete(Key key) {
    auto subscription = subscriptions_.get(key);
    if (!subscription) {
      return;
    }
    subscription->removeSignal_();
    livePublishers_.erase(key);
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
      typename PublishedObjectTrait::PublisherAttributes key,
      const std::shared_ptr<SaiObject<PublishedObjectTrait>> object) {
    std::get<PublishedObjectTrait>(publishers_).notifyCreate(key, object);
  }

  template <typename PublishedObjectTrait>
  void notifyDelete(typename PublishedObjectTrait::PublisherAttributes key) {
    std::get<PublishedObjectTrait>(publishers_).notifyDelete(key);
  }

  template <typename PublishedObjectTrait>
  detail::SaiObjectEventPublisher<PublishedObjectTrait>& get() {
    return std::get<detail::SaiObjectEventPublisher<PublishedObjectTrait>>(
        publishers_);
  }

 private:
  std::tuple<
      detail::SaiObjectEventPublisher<SaiNeighborTraits>,
      detail::SaiObjectEventPublisher<SaiIpNextHopTraits>,
      detail::SaiObjectEventPublisher<SaiMplsNextHopTraits>>
      publishers_;
};

} // namespace facebook::fboss
