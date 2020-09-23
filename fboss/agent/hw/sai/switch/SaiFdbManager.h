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

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

#include "folly/container/F14Map.h"
#include "folly/container/F14Set.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace facebook::fboss {

class ConcurrentIndices;
class SaiManagerTable;
class SaiPlatform;
class MacEntry;

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
      sai_fdb_entry_type_t type,
      std::optional<sai_uint32_t> metadata,
      bool metadataSupported)
      : Base(portId, interfaceId),
        switchId_(switchId),
        portId_(portId),
        interfaceId_(interfaceId),
        mac_(mac),
        type_(type),
        metadata_(metadata),
        metadataSupported_(metadataSupported) {}

  void createObject(PublisherObjects);
  void removeObject(size_t, PublisherObjects);
  SaiFdbTraits::FdbEntry makeFdbEntry(
      const SaiManagerTable* managerTable) const;

  PortID getPortId() const;
  L2Entry toL2Entry() const;
  InterfaceID getInterfaceID() const {
    return interfaceId_;
  }
  folly::MacAddress getMac() const {
    return mac_;
  }
  sai_fdb_entry_type_t getType() const {
    return type_;
  }
  sai_uint32_t getMetaData() const {
    return metadata_.has_value() ? metadata_.value() : 0;
  }

  void update(const std::shared_ptr<MacEntry>& updated);

 private:
  SwitchSaiId switchId_;
  PortID portId_;
  InterfaceID interfaceId_;
  folly::MacAddress mac_;
  sai_fdb_entry_type_t type_;
  std::optional<sai_uint32_t> metadata_;
  bool metadataSupported_;
};

class SaiFdbManager {
 public:
  SaiFdbManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform,
      const ConcurrentIndices* concurrentIndices);

  void addFdbEntry(
      PortID,
      InterfaceID,
      folly::MacAddress,
      sai_fdb_entry_type_t type,
      std::optional<sai_uint32_t> metadata);
  void removeFdbEntry(InterfaceID, folly::MacAddress);
  // Process mac Table changes
  void addMac(const std::shared_ptr<MacEntry>& addedEntry);
  void removeMac(const std::shared_ptr<MacEntry>& removedEntry);
  void changeMac(
      const std::shared_ptr<MacEntry>& oldEntry,
      const std::shared_ptr<MacEntry>& newEntry);
  void handleLinkDown(PortID portId);
  std::vector<L2EntryThrift> getL2Entries() const;

 private:
  L2EntryThrift fdbToL2Entry(const SaiFdbTraits::FdbEntry& fdbEntry) const;
  InterfaceID getInterfaceId(const std::shared_ptr<MacEntry>& macEntry) const;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  const ConcurrentIndices* concurrentIndices_;
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
