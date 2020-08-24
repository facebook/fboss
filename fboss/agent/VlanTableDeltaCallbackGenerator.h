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
    genTableCallbacks<folly::MacAddress>(stateDelta, cb);
    genTableCallbacks<folly::IPAddressV6>(stateDelta, cb);
    genTableCallbacks<folly::IPAddressV4>(stateDelta, cb);
  }

 private:
  template <typename AddrT, typename Callback>
  static void genTableCallbacks(const StateDelta& stateDelta, Callback& cb) {
    for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
      auto newVlan = vlanDelta.getNew();
      if (!newVlan) {
        continue;
      }
      auto vlan = newVlan->getID();

      for (const auto& delta : getTableDelta<AddrT>(vlanDelta)) {
        auto oldEntry = delta.getOld();
        auto newEntry = delta.getNew();

        if (!oldEntry) {
          cb.processAdded(stateDelta.newState(), vlan, newEntry);
        } else if (!newEntry) {
          cb.processRemoved(stateDelta.oldState(), vlan, oldEntry);
        } else {
          cb.processChanged(stateDelta, vlan, oldEntry, newEntry);
        }
      }
    }
  }
  template <typename AddrT>
  static auto getTableDelta(const VlanDelta& vlanDelta) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      return vlanDelta.getMacDelta();
    } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return vlanDelta.getArpDelta();
    } else {
      return vlanDelta.getNdpDelta();
    }
  }
};
} // namespace facebook::fboss
