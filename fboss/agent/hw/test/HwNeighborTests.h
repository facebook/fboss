// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/test/TrunkUtils.h"

#pragma once

using namespace ::testing;

namespace facebook::fboss {

template <typename AddrType, bool trunk = false>
struct NeighborT {
  using IPAddrT = AddrType;
  static constexpr auto isTrunk = trunk;
  static facebook::fboss::cfg::SwitchConfig initialConfig(
      facebook::fboss::cfg::SwitchConfig config);

  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      folly::IPAddressV4> static getNeighborAddress() {
    return folly::IPAddressV4("129.0.0.1");
  }

  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV6>::value,
      folly::IPAddressV6> static getNeighborAddress() {
    return folly::IPAddressV6("2401::123");
  }
};

using PortNeighborV4 = NeighborT<folly::IPAddressV4, false>;
using TrunkNeighborV4 = NeighborT<folly::IPAddressV4, true>;
using PortNeighborV6 = NeighborT<folly::IPAddressV6, false>;
using TrunkNeighborV6 = NeighborT<folly::IPAddressV6, true>;

const facebook::fboss::AggregatePortID kAggID{1};

template <>
facebook::fboss::cfg::SwitchConfig PortNeighborV4::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  return config;
}

void setTrunk(facebook::fboss::cfg::SwitchConfig* config) {
  std::vector<int> ports;
  for (auto port : *config->ports_ref()) {
    ports.push_back(*port.logicalID_ref());
  }
  facebook::fboss::utility::addAggPort(kAggID, ports, config);
}

template <>
facebook::fboss::cfg::SwitchConfig TrunkNeighborV4::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  setTrunk(&config);
  return config;
}

template <>
facebook::fboss::cfg::SwitchConfig PortNeighborV6::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  return config;
}

template <>
facebook::fboss::cfg::SwitchConfig TrunkNeighborV6::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  setTrunk(&config);
  return config;
}

using NeighborTypes = ::testing::
    Types<PortNeighborV4, TrunkNeighborV4, PortNeighborV6, TrunkNeighborV6>;

template <typename NeighborT>
class HwNeighborTest : public HwLinkStateDependentTest {
 protected:
  using IPAddrT = typename NeighborT::IPAddrT;
  static auto constexpr programToTrunk = NeighborT::isTrunk;
  using NTable = typename std::conditional_t<
      std::is_same<IPAddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return NeighborT::initialConfig(utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC));
  }

  PortDescriptor portDescriptor() {
    if (programToTrunk)
      return PortDescriptor(kAggID);
    return PortDescriptor(masterLogicalPortIds()[0]);
  }

  std::shared_ptr<SwitchState> addNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};
    auto neighborTable = outState->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborTable<NTable>()
                             ->modify(kVlanID, &outState);
    neighborTable->addPendingEntry(ip, kIntfID);
    return outState;
  }

  std::shared_ptr<SwitchState> removeNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};

    auto neighborTable = outState->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborTable<NTable>()
                             ->modify(kVlanID, &outState);

    neighborTable->removeEntry(ip);
    return outState;
  }

  std::shared_ptr<SwitchState> resolveNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};
    auto neighborTable = outState->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborTable<NTable>()
                             ->modify(kVlanID, &outState);

    neighborTable->updateEntry(
        ip, kNeighborMac, portDescriptor(), kIntfID, kLookupClass);
    return outState;
  }

  std::shared_ptr<SwitchState> unresolveNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    return addNeighbor(removeNeighbor(inState));
  }

  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};

  const folly::MacAddress kNeighborMac{"1:2:3:4:5:6"};
  const cfg::AclLookupClass kLookupClass{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};
};

} // namespace facebook::fboss
