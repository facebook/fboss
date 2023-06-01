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

namespace facebook::fboss {

class SwitchState;
class StateDelta;
class Vlan;

enum ArpOpCode : uint16_t;
enum class ICMPv6Type : uint8_t;

struct NeighborCaches {
  // These are shared_ptrs for safety reasons as it lets callers safely use
  // the results of getArpCacheFor or getNdpCacheFor even if the vlan is
  // deleted in another thread.
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
            std::make_shared<NdpCache>(sw, state, vlanID, vlanName, intfID)) {}
};

/**
 * This class implements the core neighbor update logic. Its methods are only
 * called from `NeighborUpdater` which schedules calls to this class on the
 * neighbor thread.
 * Having all data structures accessed only from the neighbor thread avoids
 * concurrency issues and simplifies the threading model.
 */
class NeighborUpdaterImpl {
 public:
  explicit NeighborUpdaterImpl(SwSwitch* sw);
  ~NeighborUpdaterImpl();

  // All methods other than constructor/destructor of this class are private
  // because only NeighborUpdater should be using this class and that is marked
  // as a friend class anyway.
  //
  // See comment in NeighborUpdater-defs.h
 private:
#define ARG_LIST_ENTRY(TYPE, NAME) TYPE NAME
#define NEIGHBOR_UPDATER_METHOD(VISIBILITY, NAME, RETURN_TYPE, ...) \
  RETURN_TYPE NAME(ARG_LIST(ARG_LIST_ENTRY, ##__VA_ARGS__));
#include "fboss/agent/NeighborUpdater-defs.h"
#undef NEIGHBOR_UPDATER_METHOD

  // TODO(skhare) Remove after completely migrating to intfCaches_
  std::shared_ptr<NeighborCaches> createCaches(
      const SwitchState* state,
      const Vlan* vlan);

  std::shared_ptr<NeighborCaches> createCachesForIntf(
      const SwitchState* state,
      const Interface* intf);

  void portChanged(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  void aggregatePortChanged(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);

  // TODO(skhare) Remove after completely migrating to intfCaches_
  std::shared_ptr<ArpCache> getArpCacheFor(VlanID vlan);
  std::shared_ptr<ArpCache> getArpCacheInternal(VlanID vlan);
  std::shared_ptr<NdpCache> getNdpCacheFor(VlanID vlan);
  std::shared_ptr<NdpCache> getNdpCacheInternal(VlanID vlan);

  std::shared_ptr<ArpCache> getArpCacheForIntf(InterfaceID intfID);
  std::shared_ptr<ArpCache> getArpCacheInternalForIntf(InterfaceID intfID);
  std::shared_ptr<NdpCache> getNdpCacheForIntf(InterfaceID intfID);
  std::shared_ptr<NdpCache> getNdpCacheInternalForIntf(InterfaceID intfID);

  // TODO(skhare) Remove after completely migrating to intfCaches_
  bool flushEntryImpl(VlanID vlan, folly::IPAddress ip);

  bool flushEntryImplForIntf(InterfaceID intfID, folly::IPAddress ip);

  // Forbidden copy constructor and assignment operator
  NeighborUpdaterImpl(NeighborUpdaterImpl const&) = delete;
  NeighborUpdaterImpl& operator=(NeighborUpdaterImpl const&) = delete;

  // TODO(skhare) Remove after completely migrating to intfCaches_
  boost::container::flat_map<VlanID, std::shared_ptr<NeighborCaches>> caches_;

  boost::container::flat_map<InterfaceID, std::shared_ptr<NeighborCaches>>
      intfCaches_;

  SwSwitch* sw_{nullptr};

  friend class NeighborUpdater;
};
} // namespace facebook::fboss
