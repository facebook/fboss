// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/QosTestUtils.h"
#include <folly/IPAddressV4.h>

#include "fboss/agent/state/Port.h"

namespace facebook::fboss::utility {

namespace {
template <typename AddrT>
void setDisableTTLDecrementOnNeighnor(
    std::shared_ptr<SwitchState>* state,
    InterfaceID intfID,
    AddrT ip) {
  auto intf = (*state)->getInterfaces()->getNodeIf(intfID);
  auto vlan = (*state)->getVlans()->getNodeIf(intf->getVlanID())->modify(state);

  if (vlan->template getNeighborEntryTable<AddrT>()->getEntryIf(ip)) {
    auto nbrTable =
        vlan->template getNeighborEntryTable<AddrT>()->modify(&vlan, state);
    auto entry = nbrTable->getEntryIf(ip)->clone();
    entry->setDisableTTLDecrement(true);
    nbrTable->updateNode(entry);
  } else if (intf->template getNeighborEntryTable<AddrT>()->getEntryIf(ip)) {
    auto nbrTable =
        intf->template getNeighborEntryTable<AddrT>()->modify(intfID, state);
    auto entry = nbrTable->getEntryIf(ip)->clone();
    entry->setDisableTTLDecrement(true);
    nbrTable->updateNode(entry);
  }
}
} // namespace

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    RouterID routerId,
    InterfaceID intfID,
    const folly::IPAddress& nhop) {
  // TODO: get the scope of the interface from the state and verify it is
  // supported on ASIC of that switch
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    throw FbossError("Next hop ttl decrement is not supported on this ASIC");
  }
  auto newState = state->clone();
  for (auto iter : std::as_const(*newState->getFibs())) {
    auto fibMap = iter.second;
    auto fibContainer = fibMap->getFibContainerIf(routerId);
    if (!fibContainer) {
      continue;
    }
    auto v4Fib = fibContainer->getFibV4()->modify(routerId, &newState);
    auto v6Fib = fibContainer->getFibV6()->modify(routerId, &newState);
    v4Fib->disableTTLDecrement(nhop);
    v6Fib->disableTTLDecrement(nhop);
  }

  if (nhop.isV4()) {
    setDisableTTLDecrementOnNeighnor(&newState, intfID, nhop.asV4());
  } else {
    setDisableTTLDecrementOnNeighnor(&newState, intfID, nhop.asV6());
  }
  return newState;
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
