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

#include "folly/container/F14Map.h"
#include "folly/container/F14Set.h"

#include <memory>
#include <unordered_map>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiFdbEntry = SaiObject<SaiFdbTraits>;

class ManagedFdbEntry : public SaiObjectEventAggregateSubscriber<
                            ManagedFdbEntry,
                            SaiFdbTraits,
                            SaiBridgePortTraits,
                            SaiRouterInterfaceTraits> {
 public:
  using Base = SaiObjectEventAggregateSubscriber<
      ManagedFdbEntry,
      SaiFdbTraits,
      SaiBridgePortTraits,
      SaiRouterInterfaceTraits>;

  using BridgePortWeakPtr = std::weak_ptr<const SaiObject<SaiBridgePortTraits>>;
  using RouterInterfaceWeakPtr =
      std::weak_ptr<const SaiObject<SaiRouterInterfaceTraits>>;
  using PublisherObjects =
      std::tuple<BridgePortWeakPtr, RouterInterfaceWeakPtr>;

  ManagedFdbEntry(
      SwitchSaiId switchId,
      PortID portId,
      InterfaceID interfaceId,
      const folly::MacAddress& mac,
      sai_fdb_entry_type_t type)
      : Base(portId, interfaceId),
        switchId_(switchId),
        portId_(portId),
        interfaceId_(interfaceId),
        mac_(mac),
        type_(type) {}

  void createObject(PublisherObjects);
  void removeObject(size_t, PublisherObjects);

  PortID getPortId() const;

 private:
  SwitchSaiId switchId_;
  PortID portId_;
  InterfaceID interfaceId_;
  folly::MacAddress mac_;
  sai_fdb_entry_type_t type_;
};

class SaiFdbManager {
 public:
  SaiFdbManager(SaiManagerTable* managerTable, const SaiPlatform* platform);

  void addFdbEntry(
      PortID,
      InterfaceID,
      folly::MacAddress,
      sai_fdb_entry_type_t type);
  void removeFdbEntry(InterfaceID, folly::MacAddress);

  void handleLinkDown(PortID portId);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<
      PublisherKey<SaiFdbTraits>::custom_type,
      std::shared_ptr<ManagedFdbEntry>>
      managedFdbEntries_;
  folly::F14FastMap<
      PortID,
      folly::F14FastSet<PublisherKey<SaiFdbTraits>::custom_type>>
      portToKeys_;
};

} // namespace facebook::fboss
