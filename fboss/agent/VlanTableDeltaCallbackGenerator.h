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

#include "fboss/agent/types.h"

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/VlanMapDelta.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

namespace facebook::fboss {
/*
 * Aabstraction that processes vlan delta and then
 * generates calls for any updates to Arp, Ndp, Mac
 * entries in Vlan. We need to do this in a few places,
 * so having a single abstraction doing this saves on
 * code dup.
 */

class VlanTableDeltaCallbackGenerator {
 public:
  template <typename Callback>
  static void genCallbacks(const StateDelta& stateDelta, Callback& cb) {
    genTableCallbacks<
        folly::MacAddress,
        MultiSwitchMapDelta<MultiSwitchVlanMap>>(
        stateDelta, stateDelta.getVlansDelta(), cb);
    genTableCallbacks<
        folly::IPAddressV6,
        MultiSwitchMapDelta<MultiSwitchVlanMap>>(
        stateDelta, stateDelta.getVlansDelta(), cb);
    genTableCallbacks<
        folly::IPAddressV4,
        MultiSwitchMapDelta<MultiSwitchVlanMap>>(
        stateDelta, stateDelta.getVlansDelta(), cb);
  }

  template <typename AddrT>
  static auto getTable(const std::shared_ptr<Vlan>& vlan) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      return vlan->getMacTable();
    } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return vlan->getArpTable();
    } else {
      return vlan->getNdpTable();
    }
  }

  template <typename AddrT, typename MapDeltaT>
  static auto getTableDelta(const MapDeltaT& mapDelta) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      return mapDelta.getMacDelta();
    } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return mapDelta.getArpDelta();
    } else {
      return mapDelta.getNdpDelta();
    }
  }

 private:
  template <typename AddrT, typename MapDeltaT, typename Callback>
  static void genTableCallbacks(
      const StateDelta& stateDelta,
      const MapDeltaT& mapDelta,
      Callback& cb) {
    for (const auto& entryDelta : mapDelta) {
      auto newEntry = entryDelta.getNew();
      if (!newEntry) {
        continue;
      }
      auto vlanID = newEntry->getID();

      for (const auto& delta : getTableDelta<AddrT>(entryDelta)) {
        auto oldNbrEntry = delta.getOld();
        auto newNbrEntry = delta.getNew();

        if (!oldNbrEntry) {
          cb.processAdded(stateDelta.newState(), vlanID, newNbrEntry);
        } else if (!newNbrEntry) {
          cb.processRemoved(stateDelta.oldState(), vlanID, oldNbrEntry);
        } else {
          cb.processChanged(stateDelta, vlanID, oldNbrEntry, newNbrEntry);
        }
      }
    }
  }
};
} // namespace facebook::fboss
