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

#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

#include <memory>
#include <unordered_map>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiFdbEntry = SaiObject<SaiFdbTraits>;

class SubscriberForFdbEntry : public SaiObjectEventAggregateSubscriber<
                                  SubscriberForFdbEntry,
                                  SaiFdbTraits,
                                  SaiBridgePortTraits,
                                  SaiRouterInterfaceTraits> {
 public:
  using Base = SaiObjectEventAggregateSubscriber<
      SubscriberForFdbEntry,
      SaiFdbTraits,
      SaiBridgePortTraits,
      SaiRouterInterfaceTraits>;

  using BridgePortWeakPtr = std::weak_ptr<const SaiObject<SaiBridgePortTraits>>;
  using RouterInterfaceWeakPtr =
      std::weak_ptr<const SaiObject<SaiRouterInterfaceTraits>>;
  using PublisherObjects =
      std::tuple<BridgePortWeakPtr, RouterInterfaceWeakPtr>;

  SubscriberForFdbEntry(
      SwitchSaiId switchId,
      PortID portId,
      InterfaceID interfaceId,
      const folly::MacAddress& mac)
      : Base(portId, interfaceId),
        switchId_(switchId),
        portId_(portId),
        interfaceId_(interfaceId),
        mac_(mac) {}

  void createObject(PublisherObjects);
  void removeObject(size_t, PublisherObjects);

 private:
  SwitchSaiId switchId_;
  PortID portId_;
  InterfaceID interfaceId_;
  folly::MacAddress mac_;
};

class SaiFdbManager {
 public:
  SaiFdbManager(SaiManagerTable* managerTable, const SaiPlatform* platform);

  void addFdbEntry(PortID, InterfaceID, folly::MacAddress);
  void removeFdbEntry(InterfaceID, folly::MacAddress);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<
      PublisherKey<SaiFdbTraits>::custom_type,
      std::shared_ptr<SubscriberForFdbEntry>>
      subscribersForFdbEntry_;
};

} // namespace facebook::fboss
