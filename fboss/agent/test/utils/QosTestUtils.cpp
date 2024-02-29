// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/QosTestUtils.h"

#include "fboss/agent/state/Port.h"

namespace facebook::fboss::utility {

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& /*state*/,
    RouterID /*routerId*/,
    InterfaceID /*intf*/,
    const folly::IPAddress& /*nhop*/) {
  // TODO: get the scope of the interface from the state and verify it is
  // supported on ASIC of that switch
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    throw FbossError("Next hop ttl decrement is not supported on this ASIC");
  }
  throw FbossError("Not implemented yet");
}

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    const PortDescriptor& port) {
  // TODO: get the scope of the port from the state and verify it is supported
  // on ASIC of that switch
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    throw FbossError("port ttl decrement is not supported on this ASIC");
  }
  if (!port.isPhysicalPort()) {
    throw FbossError("Cannot ttl decrement for non-physical port");
  }
  auto newState = state->clone();
  auto phyPort =
      state->getPorts()->getNodeIf(port.phyPortID())->modify(&newState);
  phyPort->setTTLDisableDecrement(true);
  return newState;
}

} // namespace facebook::fboss::utility
