// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

#include <chrono>
#include <map>
#include <vector>

namespace facebook::fboss::utility {

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhop);

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    const PortDescriptor& port);

template <typename EcmpNhopT>
std::shared_ptr<SwitchState> disableTTLDecrements(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  if (asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    return disableTTLDecrement(
        asicTable, state, routerId, nhop.intf, folly::IPAddress(nhop.ip));
  } else if (asicTable->isFeatureSupportedOnAnyAsic(
                 HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    return disableTTLDecrement(asicTable, state, nhop.portDesc);
  } else {
    throw FbossError("Disable decrement not supported");
  }
}

template <typename EcmpNhopT>
void disableTTLDecrements(
    SwSwitch* sw,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  sw->updateStateBlocking(
      "disable TTL decrement",
      [sw, &routerId, &nhop](const std::shared_ptr<SwitchState>& state) {
        auto newState = utility::disableTTLDecrements(
            sw->getHwAsicTable(), state, routerId, nhop);
        return newState;
      });
}
} // namespace facebook::fboss::utility
