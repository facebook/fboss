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

#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <fmt/format.h>
#include <memory>
#include <mutex>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiNeighborManager;
class SaiStore;

using SaiNeighbor = SaiObject<SaiNeighborTraits>;

struct SaiNeighborHandle {
  const SaiNeighbor* neighbor;
  const SaiFdbEntry* fdbEntry;
};

class ManagedVlanRifNeighbor : public SaiObjectEventAggregateSubscriber<
                                   ManagedVlanRifNeighbor,
                                   SaiNeighborTraits,
                                   SaiFdbTraits> {
 public:
  using Base = SaiObjectEventAggregateSubscriber<
      ManagedVlanRifNeighbor,
      SaiNeighborTraits,
      SaiFdbTraits>;
  using FdbWeakptr = std::weak_ptr<const SaiObject<SaiFdbTraits>>;
  using PublisherObjects = std::tuple<FdbWeakptr>;

  ManagedVlanRifNeighbor(
      SaiNeighborManager* manager,
      std::tuple<SaiPortDescriptor, RouterInterfaceSaiId> saiPortAndIntf,
      std::tuple<InterfaceID, folly::IPAddress, folly::MacAddress>
          intfIDAndIpAndMac,
      std::optional<sai_uint32_t> metadata,
      std::optional<sai_uint32_t> encapIndex,
      bool isLocal);

  void createObject(PublisherObjects objects);
  void removeObject(size_t index, PublisherObjects objects);
  void handleLinkDown();

  SaiNeighborHandle* getHandle() const {
    return handle_.get();
  }

  void notifySubscribers() const;

  std::string toString() const;

  SaiPortDescriptor getSaiPortDesc() const {
    return std::get<SaiPortDescriptor>(saiPortAndIntf_);
  }

 private:
  RouterInterfaceSaiId getRouterInterfaceSaiId() const {
    return std::get<RouterInterfaceSaiId>(saiPortAndIntf_);
  }

  SaiNeighborManager* manager_;
  std::tuple<SaiPortDescriptor, RouterInterfaceSaiId> saiPortAndIntf_;
  std::tuple<InterfaceID, folly::IPAddress, folly::MacAddress>
      intfIDAndIpAndMac_;
  std::unique_ptr<SaiNeighborHandle> handle_;
  std::optional<sai_uint32_t> metadata_;
};

class PortRifNeighbor {
 public:
  PortRifNeighbor(
      SaiNeighborManager* manager,
      std::tuple<SaiPortDescriptor, RouterInterfaceSaiId> saiPortAndIntf,
      std::tuple<InterfaceID, folly::IPAddress, folly::MacAddress>
          intfIDAndIpAndMac,
      std::optional<sai_uint32_t> metadata,
      std::optional<sai_uint32_t> encapIndex,
      bool isLocal);

  void handleLinkDown();

  SaiNeighborHandle* getHandle() const {
    return handle_.get();
  }
  void notifySubscribers() const {
    // noop
  }

  std::string toString() const {
    return fmt::format("{}", neighbor_->attributes());
  }
  SaiPortDescriptor getSaiPortDesc() const {
    return std::get<SaiPortDescriptor>(saiPortAndIntf_);
  }

 private:
  RouterInterfaceSaiId getRouterInterfaceSaiId() const {
    return std::get<RouterInterfaceSaiId>(saiPortAndIntf_);
  }

  SaiNeighborManager* manager_;
  std::tuple<SaiPortDescriptor, RouterInterfaceSaiId> saiPortAndIntf_;
  std::shared_ptr<SaiNeighbor> neighbor_;
  std::unique_ptr<SaiNeighborHandle> handle_;
};

class SaiNeighborEntry {
 public:
  SaiNeighborEntry(
      SaiNeighborManager* manager,
      std::tuple<SaiPortDescriptor, RouterInterfaceSaiId> saiPortAndIntf,
      std::tuple<InterfaceID, folly::IPAddress, folly::MacAddress>
          intfIDAndIpAndMac,
      std::optional<sai_uint32_t> metadata,
      std::optional<sai_uint32_t> encapIndex,
      bool isLocal,
      cfg::InterfaceType intfType);
  void handleLinkDown() {
    std::visit([](auto& handle) { handle->handleLinkDown(); }, neighbor_);
  }

  cfg::InterfaceType getRifType() const;
  SaiNeighborHandle* getHandle() const {
    return std::visit(
        [](auto& handle) { return handle->getHandle(); }, neighbor_);
  }
  void notifySubscribers() const {
    std::visit([](auto& handle) { handle->notifySubscribers(); }, neighbor_);
  }

  std::string toString() const {
    return std::visit(
        [](auto& handle) { return handle->toString(); }, neighbor_);
  }
  SaiPortDescriptor getSaiPortDesc() const {
    return std::visit(
        [](auto& handle) { return handle->getSaiPortDesc(); }, neighbor_);
  }

 private:
  std::variant<
      std::shared_ptr<ManagedVlanRifNeighbor>,
      std::shared_ptr<PortRifNeighbor>>
      neighbor_;
};

class SaiNeighborManager {
 public:
  SaiNeighborManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  // Helper function to create a SAI NeighborEntry from an FBOSS SwitchState
  // NeighborEntry (e.g., NeighborEntry<IPAddressV6, NDPTable>)
  template <typename NeighborEntryT>
  SaiNeighborTraits::NeighborEntry saiEntryFromSwEntry(
      const std::shared_ptr<NeighborEntryT>& swEntry);

  template <typename NeighborEntryT>
  void changeNeighbor(
      const std::shared_ptr<NeighborEntryT>& oldSwEntry,
      const std::shared_ptr<NeighborEntryT>& newSwEntry);

  template <typename NeighborEntryT>
  void addNeighbor(const std::shared_ptr<NeighborEntryT>& swEntry);

  template <typename NeighborEntryT>
  void removeNeighbor(const std::shared_ptr<NeighborEntryT>& swEntry);

  SaiNeighborHandle* getNeighborHandle(
      const SaiNeighborTraits::NeighborEntry& entry);
  const SaiNeighborHandle* getNeighborHandle(
      const SaiNeighborTraits::NeighborEntry& entry) const;

  cfg::InterfaceType getNeighborRifType(
      const SaiNeighborTraits::NeighborEntry& entry) const;

  void clear();

  std::shared_ptr<SaiNeighbor> createSaiObject(
      const SaiNeighborTraits::AdapterHostKey& key,
      const SaiNeighborTraits::CreateAttributes& attributes);

  std::string listManagedObjects() const;
  SwitchSaiId getSwitchSaiId() const;

  void handleLinkDown(const SaiPortDescriptor& /*port*/);

 private:
  SaiNeighborHandle* getNeighborHandleImpl(
      const SaiNeighborTraits::NeighborEntry& entry) const;

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<
      SaiNeighborTraits::NeighborEntry,
      std::unique_ptr<SaiNeighborEntry>>
      neighbors_;
};

} // namespace facebook::fboss
