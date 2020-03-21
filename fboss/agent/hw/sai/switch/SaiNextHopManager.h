/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

#include "folly/IPAddress.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class ResolvedNextHop;

using SaiIpNextHop = SaiObject<SaiIpNextHopTraits>;
using SaiMplsNextHop = SaiObject<SaiMplsNextHopTraits>;
using SaiNextHop = typename ConditionSaiObjectType<SaiNextHopTraits>::type;

template <typename NextHopTraits>
class SaiNeighborSubscriberForNextHop
    : public SaiObjectEventAggregateSubscriber<
          SaiNeighborSubscriberForNextHop<NextHopTraits>,
          NextHopTraits,
          SaiNeighborTraits> {
 public:
  using AdapterHostKey = typename NextHopTraits::AdapterHostKey;
  using NeighborWeakPtr = std::weak_ptr<const SaiObject<SaiNeighborTraits>>;
  using PublishedObjects = std::tuple<NeighborWeakPtr>;
  using Base = SaiObjectEventAggregateSubscriber<
      SaiNeighborSubscriberForNextHop<NextHopTraits>,
      NextHopTraits,
      SaiNeighborTraits>;
  SaiNeighborSubscriberForNextHop(
      SaiNeighborTraits::NeighborEntry entry,
      const typename NextHopTraits::AdapterHostKey& key)
      : Base(entry), key_(key) {}

  void createObject(PublishedObjects /*added*/) {
    CHECK(this->allPublishedObjectsAlive()) << "neighbors are not ready";

    /* when neighbor is created setup next hop */
    if constexpr (std::is_same_v<NextHopTraits, SaiIpNextHopTraits>) {
      this->setObject(
          key_,
          {SAI_NEXT_HOP_TYPE_IP,
           std::get<typename NextHopTraits::Attributes::RouterInterfaceId>(
               key_),
           std::get<typename NextHopTraits::Attributes::Ip>(key_)});

    } else {
      this->setObject(
          key_,
          {SAI_NEXT_HOP_TYPE_MPLS,
           std::get<typename NextHopTraits::Attributes::RouterInterfaceId>(
               key_),
           std::get<typename NextHopTraits::Attributes::Ip>(key_),
           std::get<typename NextHopTraits::Attributes::LabelStack>(key_)});
    }
  }

  void removeObject(size_t index, PublishedObjects removed) {
    /* when neighbor is removed remove next hop */
    this->resetObject();
  }

 private:
  typename NextHopTraits::AdapterHostKey key_;
};

using SubscriberForIpNextHop =
    SaiNeighborSubscriberForNextHop<SaiIpNextHopTraits>;
using SubscriberForMplsNextHop =
    SaiNeighborSubscriberForNextHop<SaiMplsNextHopTraits>;

using SaiNeighborSubscriber = std::variant<
    std::shared_ptr<SubscriberForIpNextHop>,
    std::shared_ptr<SubscriberForMplsNextHop>>;

class SaiNextHopManager {
 public:
  SaiNextHopManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  std::shared_ptr<SaiIpNextHop> addNextHop(
      RouterInterfaceSaiId routerInterfaceId,
      const folly::IPAddress& ip);

  /* return next hop handle here, based on resolved next hop. resulting next hop
   * handle will be either ip next hop or mpls next hop depending on properties
   * of resolved next hop */
  // TODO: remove below function
  SaiNextHop refOrEmplace(const ResolvedNextHop& swNextHop);

  SaiNextHopTraits::AdapterHostKey getAdapterHostKey(
      const ResolvedNextHop& swNextHop);

  SaiNeighborSubscriber refOrEmplaceSubscriber(
      const ResolvedNextHop& swNextHop);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

  UnorderedRefMap<
      typename SubscriberForIpNextHop::AdapterHostKey,
      SubscriberForIpNextHop>
      ipSubscribers_;
  UnorderedRefMap<
      typename SubscriberForMplsNextHop::AdapterHostKey,
      SubscriberForMplsNextHop>
      mplsSubscribers_;
};

} // namespace facebook::fboss
