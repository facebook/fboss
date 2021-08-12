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
class SaiFdbManager;
class SaiStore;

using SaiFdbEntry = SaiObject<SaiFdbTraits>;

class ManagedFdbEntry : public SaiObjectEventAggregateSubscriber<
                            ManagedFdbEntry,
                            SaiFdbTraits,
                            SaiBridgePortTraits,
                            SaiVlanRouterInterfaceTraits> {
 public:
  using Base = SaiObjectEventAggregateSubscriber<
      ManagedFdbEntry,
      SaiFdbTraits,
      SaiBridgePortTraits,
      SaiVlanRouterInterfaceTraits>;

  using BridgePortWeakPtr = std::weak_ptr<const SaiObject<SaiBridgePortTraits>>;
  using RouterInterfaceWeakPtr =
      std::weak_ptr<const SaiObject<SaiVlanRouterInterfaceTraits>>;
  using PublisherObjects =
      std::tuple<BridgePortWeakPtr, RouterInterfaceWeakPtr>;

  ManagedFdbEntry(
      SaiFdbManager* manager,
      SwitchSaiId switchId,
      SaiPortDescriptor portId,
      InterfaceID interfaceId,
      const folly::MacAddress& mac,
      sai_fdb_entry_type_t type,
      std::optional<sai_uint32_t> metadata)
      : Base(portId, interfaceId),
        manager_(manager),
        switchId_(switchId),
        portId_(portId),
        interfaceId_(interfaceId),
        mac_(mac),
        type_(type),
        metadata_(metadata) {}

  void createObject(PublisherObjects);
  void removeObject(size_t, PublisherObjects);
  SaiFdbTraits::FdbEntry makeFdbEntry(
      const SaiManagerTable* managerTable) const;
  void handleLinkDown();

  SaiPortDescriptor getPortId() const;
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
  SaiFdbManager* manager_;
  SwitchSaiId switchId_;
  SaiPortDescriptor portId_;
  InterfaceID interfaceId_;
  folly::MacAddress mac_;
  sai_fdb_entry_type_t type_;
  std::optional<sai_uint32_t> metadata_;
};

class SaiFdbManager {
 public:
  SaiFdbManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform,
      const ConcurrentIndices* concurrentIndices);

  void addFdbEntry(
      SaiPortDescriptor,
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
  void handleLinkDown(SaiPortDescriptor portId);
  std::vector<L2EntryThrift> getL2Entries() const;

  std::shared_ptr<SaiFdbEntry> createSaiObject(
      const typename SaiFdbTraits::AdapterHostKey& key,
      const typename SaiFdbTraits::CreateAttributes& attributes,
      const PublisherKey<SaiFdbTraits>::custom_type& publisherKey);

  void removeUnclaimedDynanicEntries();

 private:
  PortDescriptorSaiId getPortDescriptorSaiId(
      const PortDescriptor& portDesc) const;
  L2EntryThrift fdbToL2Entry(const SaiFdbTraits::FdbEntry& fdbEntry) const;
  InterfaceID getInterfaceId(const std::shared_ptr<MacEntry>& macEntry) const;

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  const ConcurrentIndices* concurrentIndices_;
  folly::F14FastMap<
      PublisherKey<SaiFdbTraits>::custom_type,
      std::shared_ptr<ManagedFdbEntry>>
      managedFdbEntries_;
  folly::F14FastMap<
      SaiPortDescriptor,
      folly::F14FastSet<PublisherKey<SaiFdbTraits>::custom_type>>
      portToKeys_;
};

} // namespace facebook::fboss
