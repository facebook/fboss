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

#include <boost/container/flat_map.hpp>
#include <list>
#include <mutex>
#include <string>
#include "fboss/agent/ArpCache.h"
#include "fboss/agent/NdpCache.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {

class SwitchState;
class StateDelta;
class Vlan;

enum ArpOpCode : uint16_t;
enum class ICMPv6Type : uint8_t;

/**
 * This class handles all updates to neighbor entries. Whenever we perform an
 * action or receive a packet that should update the Arp or Ndp tables in the
 * SwitchState, it goes through this class first. This class stores Arp and NDP
 * caches for each vlan and those are the source of truth for neighbor entries.
 *
 * Those caches are self-managing and manage all changes to the Neighbor Tables
 * due to expiration or failed resolution.
 *
 * This class implements the StateObserver API and listens for all vlan added or
 * deleted events. It ignores all changes it receives related to arp/ndp tables,
 * as all those changes should originate from the caches stored in this class.
 */
class NeighborUpdaterImpl {
  struct NeighborCaches {
    /* These are shared_ptrs for safety reasons as it lets callers
     * safely use the results of getArpCacheFor or getNdpCacheFor
     * even if the vlan is deleted in another thread. */
    std::shared_ptr<ArpCache> arpCache;
    std::shared_ptr<NdpCache> ndpCache;

    NeighborCaches(
        SwSwitch* sw,
        const SwitchState* state,
        VlanID vlanID,
        std::string vlanName,
        InterfaceID intfID)
        : arpCache(
              std::make_shared<ArpCache>(sw, state, vlanID, vlanName, intfID)),
          ndpCache(
              std::make_shared<NdpCache>(sw, state, vlanID, vlanName, intfID)) {
    }
    void clearEntries() {
      arpCache->clearEntries();
      ndpCache->clearEntries();
    }
  };

 public:
  explicit NeighborUpdaterImpl();
  ~NeighborUpdaterImpl();

  // All methods other than constructor/destructor of this class are private
  // because only NeighborUpdater should be using this class and that is marked
  // as a friend class anyway.
 private:
#define ARG_LIST_ENTRY(TYPE, NAME) TYPE NAME
#define NEIGHBOR_UPDATER_METHOD(VISIBILITY, NAME, RETURN_TYPE, ...) \
  RETURN_TYPE NAME(ARG_LIST(ARG_LIST_ENTRY, ##__VA_ARGS__));
#include "fboss/agent/NeighborUpdater.def"
#undef NEIGHBOR_UPDATER_METHOD

  void portChanged(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  void aggregatePortChanged(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);

  std::shared_ptr<ArpCache> getArpCacheFor(VlanID vlan);
  std::shared_ptr<ArpCache> getArpCacheInternal(VlanID vlan);
  std::shared_ptr<NdpCache> getNdpCacheFor(VlanID vlan);
  std::shared_ptr<NdpCache> getNdpCacheInternal(VlanID vlan);

  bool flushEntryImpl(VlanID vlan, folly::IPAddress ip);

  // Forbidden copy constructor and assignment operator
  NeighborUpdaterImpl(NeighborUpdaterImpl const&) = delete;
  NeighborUpdaterImpl& operator=(NeighborUpdaterImpl const&) = delete;

  /**
   * caches_ can be accessed from multiple threads, so we need to lock accesses
   * with cachesMutex_. Note that this means that the cache implmentation cade
   * should NOT ever call back into NeighborUpdaterImpl because of this, as it
   * would likely be a lock ordering issue.
   */
  boost::container::flat_map<VlanID, std::shared_ptr<NeighborCaches>> caches_;
  std::mutex cachesMutex_;

  friend class NeighborUpdater;
};

} // namespace fboss
} // namespace facebook
