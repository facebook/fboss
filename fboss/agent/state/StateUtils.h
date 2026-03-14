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

#include <memory>
#include <string>
#include <vector>

#include "fboss/agent/types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

namespace facebook::fboss {
class SwitchState;
class Interface;
class StateDelta;

namespace utility {

/**
 * Utility functions for InterfaceID <-> ifName (on host)
 */
std::string createTunIntfName(InterfaceID ifID);
bool isTunIntfName(const std::string& ifName);
InterfaceID getIDFromTunIntfName(const std::string& ifName);

template <typename MswitchMapT>
auto getFirstMap(const std::shared_ptr<MswitchMapT>& map) {
  return map->size() ? map->cbegin()->second : nullptr;
}

template <typename MultiSwitchMapT>
auto getFirstNodeIf(const std::shared_ptr<MultiSwitchMapT>& map) {
  return map->size() ? map->cbegin()->second : nullptr;
}

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf);
folly::MacAddress getMacForFirstInterfaceWithPorts(
    const std::shared_ptr<SwitchState>& state,
    std::optional<SwitchID> switchId = std::nullopt);
InterfaceID firstInterfaceIDWithPorts(
    const std::shared_ptr<SwitchState>& state,
    std::optional<SwitchID> switchId = std::nullopt);
std::shared_ptr<Interface> firstInterfaceWithPorts(
    const std::shared_ptr<SwitchState>& state);
std::vector<folly::IPAddress> getIntfAddrs(
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& intf);
std::vector<folly::IPAddressV4> getIntfAddrsV4(
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& intf);
std::vector<folly::IPAddressV6> getIntfAddrsV6(
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& intf);

/*
 * Compute reversed state deltas for rollback.
 *
 * Given a vector of OperDeltas, the current programmed state, and the
 * intermediate state (the last successfully applied state before failure),
 * this function computes the reversed StateDelta vector needed to rollback
 * the hardware state.
 *
 * The returned deltas are in reverse order up to the intermediate state,
 * suitable for use in rollback operations.
 *
 * @param operDeltas The vector of OperDeltas that were being applied
 * @param currentState The programmed state before the deltas were applied
 * @param intermediateState The last successfully applied state before failure
 * @return Vector of StateDelta objects in reverse order for rollback
 */
std::vector<StateDelta> computeReversedDeltas(
    const std::vector<fsdb::OperDelta>& operDeltas,
    const std::shared_ptr<SwitchState>& currentState,
    const std::shared_ptr<SwitchState>& intermediateState);

} // namespace utility
} // namespace facebook::fboss
