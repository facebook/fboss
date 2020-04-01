// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <algorithm>
#include <any>
#include <memory>

#include "fboss/agent/hw/sai/store/Traits.h"
#include "fboss/lib/TupleUtils.h"

namespace facebook::fboss {

template <typename>
class SaiObject;

namespace detail {
/*
 * A subscriber interface as used by  publisher
 * afterCreate and beforeRemove methods are invoked by publishers after and
 * before publishers are created or removed respectively.
 * saveSubscription  is invoked by publisher to save subscription.
 */
template <typename PublisherObjectTraits>
struct SaiObjectEventSubscriber {
  using PublisherObjectSharedPtr =
      std::shared_ptr<const SaiObject<PublisherObjectTraits>>;
  using PublisherObjectWeakPtr =
      std::weak_ptr<const SaiObject<PublisherObjectTraits>>;

  SaiObjectEventSubscriber(
      typename PublisherAttributes<PublisherObjectTraits>::type attr);
  virtual ~SaiObjectEventSubscriber();
  typename PublisherAttributes<PublisherObjectTraits>::type
  getPublisherAttributes() const {
    return publisherAttrs_;
  }

  /* return non-owning reference to monitored object */
  PublisherObjectWeakPtr getPublisherObject() const;

  virtual void afterCreate(PublisherObjectSharedPtr) = 0;
  virtual void beforeRemove() = 0;

  void saveSubscription(std::any subscription) {
    subscription_ = std::move(subscription);
  }

 protected:
  void setPublisherObject(PublisherObjectSharedPtr object = nullptr);

 private:
  typename PublisherAttributes<PublisherObjectTraits>::type publisherAttrs_;
  PublisherObjectWeakPtr publisherObject_;
  // TODO(pshaikh): this is currently maintained as any to break circular
  // dependencies in object, publisher, and subscriber types investigate and
  // eliminate this any type with proper type
  std::any subscription_;
};

/* A single subscriber for a publisher using particular published object trait.
 * A single subscriber implements methods of afterCreate and beforeRemove.
 * Both these methods simply dispatch events from publisher to aggregate
 * subscriber. In doing so, single subscriber is presenting a facade to a
 * publisher with published trait to notify an aggregate subscriber.  */
template <typename AggregateSubscriber, typename PublishedObjectTrait>
class SaiObjectEventSingleSubscriber
    : public SaiObjectEventSubscriber<PublishedObjectTrait> {
 public:
  using PublisherObjectSharedPtr =
      std::shared_ptr<const SaiObject<PublishedObjectTrait>>;
  using Base = SaiObjectEventSubscriber<PublishedObjectTrait>;
  /* Publisher object with PublishedObjectTrait and can be identified by
   * PublisherAttributes */
  SaiObjectEventSingleSubscriber(
      typename PublisherAttributes<PublishedObjectTrait>::type monitoredAttrs)
      : Base{monitoredAttrs} {}

  /* Publisher object created, dispatch call to aggregate subscriber */
  void afterCreate(PublisherObjectSharedPtr object) override {
    this->setPublisherObject(object);
    auto* aggregateSubscriber = static_cast<AggregateSubscriber*>(this);
    aggregateSubscriber
        ->template afterCreateNotifyAggregateSubscriber<PublishedObjectTrait>();
  }

  /* Publisher object removed, dispatch call  to aggregate subscriber */
  void beforeRemove() override {
    auto* aggregateSubscriber = static_cast<AggregateSubscriber*>(this);
    aggregateSubscriber->template beforeRemoveNotifyAggregateSubscriber<
        PublishedObjectTrait>();
    this->setPublisherObject(nullptr);
  }
};
} // namespace detail

/*
 * An aggregate subscriber to aggregate events from more than one publishers
 * It aggregates all events from all publishers, manages some book keeping about
 * which publisher are alive. The notification is further dispatched to
 * implementation of subscribers.
 * An aggregate subscriber implements following methods
 *  1) afterCreateNotifyAggregateSubscriber
 *   - notify subscriber implementation if all publishers are alive
 *  2) beforeRemoveNotifyAggregateSubscriber
 *   - notify subscriber implementation if any publisher is dead
 *  3) setObject
 *    - A method for subscriber to setup SAI object
 *  4) resetObject
 *    - A method for subscriber to remove SAI objet
 *
 * A subscriber implementation in implements following methods
 *   1) createObject
 *    - create SAI object by assembling attributes from all publishers and
 *      invoke setObject
 * 2) removeObject
 *    - either call createObject with new attributes or reset object
 */
template <
    typename SubscriberImpl,
    typename SubscriberTraits,
    typename... PublishedObjectTraits>
class SaiObjectEventAggregateSubscriber
    : public detail::SaiObjectEventSingleSubscriber<
          SaiObjectEventAggregateSubscriber<
              SubscriberImpl,
              SubscriberTraits,
              PublishedObjectTraits...>,
          PublishedObjectTraits>... {
 public:
  static_assert(
      (IsObjectPublisher<PublishedObjectTraits>::value && ... && true),
      "requested object is not monitored");

  static constexpr size_t Size = sizeof...(PublishedObjectTraits);
  using AggregateType = std::tuple<PublishedObjectTraits...>;
  using AdapterHostKey = typename SubscriberTraits::AdapterHostKey;
  using CreateAttributes = typename SubscriberTraits::CreateAttributes;
  using Class = SaiObjectEventAggregateSubscriber<
      SubscriberImpl,
      SubscriberTraits,
      PublishedObjectTraits...>;
  using SubscriberSharedPtr = std::shared_ptr<SaiObject<SubscriberTraits>>;

  SaiObjectEventAggregateSubscriber(
      typename PublisherAttributes<PublishedObjectTraits>::type... attrs)
      : detail::SaiObjectEventSingleSubscriber<Class, PublishedObjectTraits>(
            attrs)... {}

  template <typename PublishedObjectTrait>
  void afterCreateNotifyAggregateSubscriber() {
    auto index = TupleIndex<AggregateType, PublishedObjectTrait>::value;
    livePublisherObjects_[index] = true;
    if (allPublishedObjectsAlive()) {
      // trigger all creation
      auto tuple = std::make_tuple(
          detail::SaiObjectEventSingleSubscriber<Class, PublishedObjectTraits>::
              getPublisherObject()...);
      auto* subscriber = static_cast<SubscriberImpl*>(this);
      subscriber->createObject(tuple);
    }
  }

  template <typename PublishedObjectTrait>
  void beforeRemoveNotifyAggregateSubscriber() {
    auto index = TupleIndex<AggregateType, PublishedObjectTrait>::value;
    livePublisherObjects_[index] = false;
    auto tuple = std::make_tuple(
        detail::SaiObjectEventSingleSubscriber<Class, PublishedObjectTraits>::
            getPublisherObject()...);
    auto* subscriber = static_cast<SubscriberImpl*>(this);
    subscriber->removeObject(index, tuple);
  }

  bool allPublishedObjectsAlive() const {
    return std::all_of(
        livePublisherObjects_.begin(),
        livePublisherObjects_.end(),
        [](auto live) { return live; });
  }

  bool isAlive() const;

 protected:
  void setObject(AdapterHostKey key, CreateAttributes attr);

  void resetObject();

 private:
  std::vector<bool> livePublisherObjects_{Size};
  SubscriberSharedPtr object_;
};

} // namespace facebook::fboss
