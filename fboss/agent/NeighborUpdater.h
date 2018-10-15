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
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/types.h"
#include "fboss/agent/ArpCache.h"
#include "fboss/agent/NdpCache.h"
#include "fboss/agent/state/PortDescriptor.h"
#include <list>
#include <mutex>
#include <string>

namespace facebook { namespace fboss {

class NeighborUpdaterImpl;
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
class NeighborUpdater : public AutoRegisterStateObserver {
 public:
  explicit NeighborUpdater(SwSwitch* sw);
  ~NeighborUpdater() override;

  void stateUpdated(const StateDelta& delta) override;

  uint32_t flushEntry (VlanID vlan, folly::IPAddress ip);

  // Ndp events
  void sentNeighborSolicitation(VlanID vlan,
                                folly::IPAddressV6 ip);
  void receivedNdpMine(
      VlanID vlan,
      folly::IPAddressV6 ip,
      folly::MacAddress mac,
      PortDescriptor port,
      ICMPv6Type type,
      uint32_t flags);
  void receivedNdpNotMine(
      VlanID vlan,
      folly::IPAddressV6 ip,
      folly::MacAddress mac,
      PortDescriptor port,
      ICMPv6Type type,
      uint32_t flags);

  // Arp events
  void sentArpRequest(VlanID vlan,
                      folly::IPAddressV4 ip);
  void receivedArpMine(VlanID vlan,
                       folly::IPAddressV4 ip,
                       folly::MacAddress mac,
                       PortDescriptor port,
                       ArpOpCode op);
  void receivedArpNotMine(VlanID vlan,
                          folly::IPAddressV4 ip,
                          folly::MacAddress mac,
                          PortDescriptor port,
                          ArpOpCode op);

  void portDown(PortDescriptor port);

  void getArpCacheData(std::vector<ArpEntryThrift>& arpTable);

  void getNdpCacheData(std::vector<NdpEntryThrift>& ndpTable);

 private:
  void vlanAdded(const SwitchState* state, const Vlan* vlan);
  void vlanDeleted(const Vlan* vlan);
  void vlanChanged(const Vlan* oldVlan, const Vlan* newVlan);

  void portChanged(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  void aggregatePortChanged(
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort);

  void sendNeighborUpdates(const VlanDelta& delta);

  std::shared_ptr<ArpCache> getArpCacheFor(VlanID vlan);
  std::shared_ptr<ArpCache> getArpCacheInternal(VlanID vlan);
  std::shared_ptr<NdpCache> getNdpCacheFor(VlanID vlan);
  std::shared_ptr<NdpCache> getNdpCacheInternal(VlanID vlan);

  bool flushEntryImpl (VlanID vlan, folly::IPAddress ip);

  // Forbidden copy constructor and assignment operator
  NeighborUpdater(NeighborUpdater const &) = delete;
  NeighborUpdater& operator=(NeighborUpdater const &) = delete;

  struct NeighborCaches {
    /* These are shared_ptrs for safety reasons as it lets callers
     * safely use the results of getArpCacheFor or getNdpCacheFor
     * even if the vlan is deleted in another thread. */
    std::shared_ptr<ArpCache> arpCache;
    std::shared_ptr<NdpCache> ndpCache;

    NeighborCaches(SwSwitch* sw, const SwitchState* state,
                   VlanID vlanID, std::string vlanName, InterfaceID intfID) :
        arpCache(
            std::make_shared<ArpCache>(sw, state, vlanID, vlanName, intfID)),
        ndpCache(
            std::make_shared<NdpCache>(sw, state, vlanID, vlanName, intfID)) {}
    void clearEntries() {
      arpCache->clearEntries();
      ndpCache->clearEntries();
    }
  };

  /**
   * caches_ can be accessed from multiple threads, so we need to lock accesses
   * with cachesMutex_. Note that this means that the cache implmentation cade
   * should NOT ever call back into NeighborUpdater because of this, as it would
   * likely be a lock ordering issue.
   */
  boost::container::flat_map<VlanID, std::shared_ptr<NeighborCaches>> caches_;
  std::mutex cachesMutex_;
  SwSwitch* sw_{nullptr};
};

}} // facebook::fboss
