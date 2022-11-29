// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <memory>

namespace facebook::fboss {

using folly::IPAddressV4;
using folly::IPAddressV6;
using std::shared_ptr;

template <typename IPTYPE>
struct NeighborEntryTestUtil {
  template <typename T = IPTYPE>
  static DeltaValue<NdpEntry> getNeighborEntryDelta(
      const StateDelta& delta,
      typename std::
          enable_if<std::is_same<T, IPAddressV6>::value, IPAddressV6>::type ip,
      VlanID vlan) {
    shared_ptr<NdpEntry> oldEntry = nullptr;
    shared_ptr<NdpEntry> newEntry = nullptr;
    const auto& newVlan = delta.getVlansDelta().getNew()->getNodeIf(vlan);
    const auto& oldVlan = delta.getVlansDelta().getOld()->getNodeIf(vlan);
    if (nullptr != newVlan) {
      newEntry = newVlan->getNdpTable()->getEntryIf(ip);
    }
    if (nullptr != oldVlan) {
      oldEntry = oldVlan->getNdpTable()->getEntryIf(ip);
    }
    return DeltaValue<NdpEntry>(oldEntry, newEntry);
  }

  template <typename T = IPTYPE>
  static DeltaValue<ArpEntry> getNeighborEntryDelta(
      const StateDelta& delta,
      typename std::
          enable_if<std::is_same<T, IPAddressV4>::value, IPAddressV4>::type ip,
      VlanID vlan) {
    shared_ptr<ArpEntry> oldEntry = nullptr;
    shared_ptr<ArpEntry> newEntry = nullptr;
    const auto& newVlan = delta.getVlansDelta().getNew()->getNodeIf(vlan);
    const auto& oldVlan = delta.getVlansDelta().getOld()->getNodeIf(vlan);
    if (nullptr != newVlan) {
      newEntry = newVlan->getArpTable()->getEntryIf(ip);
    }
    if (nullptr != oldVlan) {
      oldEntry = oldVlan->getArpTable()->getEntryIf(ip);
    }
    return DeltaValue<ArpEntry>(oldEntry, newEntry);
  }

  template <typename T = IPTYPE>
  static DeltaValue<NdpEntry> getNeighborEntryDelta(
      const StateDelta& delta,
      typename std::
          enable_if<std::is_same<T, IPAddressV6>::value, IPAddressV6>::type ip,
      InterfaceID intf) {
    shared_ptr<NdpEntry> oldEntry = nullptr;
    shared_ptr<NdpEntry> newEntry = nullptr;
    const auto& newInterface = delta.getIntfsDelta().getNew()->getNodeIf(intf);
    const auto& oldInterface = delta.getIntfsDelta().getOld()->getNodeIf(intf);
    if (nullptr != newInterface) {
      newEntry = newInterface->getNdpTable()->getEntryIf(ip);
    }
    if (nullptr != oldInterface) {
      oldEntry = oldInterface->getNdpTable()->getEntryIf(ip);
    }
    return DeltaValue<NdpEntry>(oldEntry, newEntry);
  }

  template <typename T = IPTYPE>
  static DeltaValue<ArpEntry> getNeighborEntryDelta(
      const StateDelta& delta,
      typename std::
          enable_if<std::is_same<T, IPAddressV4>::value, IPAddressV4>::type ip,
      InterfaceID intf) {
    shared_ptr<ArpEntry> oldEntry = nullptr;
    shared_ptr<ArpEntry> newEntry = nullptr;
    const auto& newInterface = delta.getIntfsDelta().getNew()->getNodeIf(intf);
    const auto& oldInterface = delta.getIntfsDelta().getOld()->getNodeIf(intf);
    if (nullptr != newInterface) {
      newEntry = newInterface->getArpTable()->getEntryIf(ip);
    }
    if (nullptr != oldInterface) {
      oldEntry = oldInterface->getArpTable()->getEntryIf(ip);
    }
    return DeltaValue<ArpEntry>(oldEntry, newEntry);
  }
};

template <typename DeltaVal>
bool oldHasEncapIndx(const DeltaVal& delta) {
  return delta.getOld() && delta.getOld()->getEncapIndex().has_value();
}
template <typename DeltaVal>
bool newHasEncapIndx(const DeltaVal& delta) {
  return delta.getNew() && delta.getNew()->getEncapIndex().has_value();
}
template <class IPTYPE>
class WaitForNeighborEntryExpiration : public WaitForSwitchState {
  template <typename NeighborEntryDelta>
  bool checkNeighborEntries(
      const NeighborEntryDelta& neighborEntryDelta) const {
    const auto& oldEntry = neighborEntryDelta.getOld();
    const auto& newEntry = neighborEntryDelta.getNew();
    return (oldEntry != nullptr) && (newEntry == nullptr);
  }

 public:
  WaitForNeighborEntryExpiration(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1))
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);
              return this->checkNeighborEntries(neighborEntryDelta);
            },
            "WaitForNeighborEntryExpiration@" + ipAddress.str()) {}
  WaitForNeighborEntryExpiration(
      SwSwitch* sw,
      IPTYPE ipAddress,
      InterfaceID intf)
      : WaitForSwitchState(
            sw,
            [ipAddress, intf, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, intf);
              return this->checkNeighborEntries(neighborEntryDelta);
            },
            "WaitForNeighborEntryExpiration@" + ipAddress.str()) {}
  ~WaitForNeighborEntryExpiration() {}
};

template <class IPTYPE>
class WaitForNeighborEntryCreation : public WaitForSwitchState {
  template <typename NeighborEntryDelta>
  bool checkNeighborEntries(
      const NeighborEntryDelta& neighborEntryDelta,
      bool pending,
      bool checkEncapIndex = true) const {
    const auto& oldEntry = neighborEntryDelta.getOld();
    const auto& newEntry = neighborEntryDelta.getNew();
    if (!newEntry) {
      return false;
    }
    if (checkEncapIndex && (!pending && !newHasEncapIndx(neighborEntryDelta))) {
      return false;
    }
    return (oldEntry == nullptr) && newEntry->isPending() == pending;
  }

 public:
  WaitForNeighborEntryCreation(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1),
      bool pending = true)
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan, pending, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);
              return this->checkNeighborEntries(neighborEntryDelta, pending);
            },
            "WaitForNeighborEntryCreation@" + ipAddress.str()) {}
  WaitForNeighborEntryCreation(
      SwSwitch* sw,
      IPTYPE ipAddress,
      InterfaceID intf,
      bool pending = true)
      : WaitForSwitchState(
            sw,
            [ipAddress, intf, pending, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, intf);
              return this->checkNeighborEntries(neighborEntryDelta, pending);
            },
            "WaitForNeighborEntryCreation@" + ipAddress.str()) {}
  ~WaitForNeighborEntryCreation() {}
};

template <class IPTYPE>
class WaitForNeighborEntryPending : public WaitForSwitchState {
  template <typename NeighborEntryDelta>
  bool checkNeighborEntries(
      const NeighborEntryDelta& neighborEntryDelta,
      bool checkEncapIndex = true) const {
    const auto& oldEntry = neighborEntryDelta.getOld();
    const auto& newEntry = neighborEntryDelta.getNew();
    if (checkEncapIndex &&
        (!oldHasEncapIndx(neighborEntryDelta) ||
         newHasEncapIndx(neighborEntryDelta))) {
      return false;
    }
    return (oldEntry != nullptr) && (newEntry != nullptr) &&
        (!oldEntry->isPending()) && (newEntry->isPending());
  }

 public:
  WaitForNeighborEntryPending(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1))
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);
              return this->checkNeighborEntries(neighborEntryDelta);
            },
            "WaitForNeighborEntryPending@" + ipAddress.str()) {}
  WaitForNeighborEntryPending(SwSwitch* sw, IPTYPE ipAddress, InterfaceID intf)
      : WaitForSwitchState(
            sw,
            [ipAddress, intf, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, intf);
              return this->checkNeighborEntries(neighborEntryDelta);
            },
            "WaitForNeighborEntryPending@" + ipAddress.str()) {}
  ~WaitForNeighborEntryPending() {}
};

template <class IPTYPE>
class WaitForNeighborEntryReachable : public WaitForSwitchState {
  template <typename NeighborEntryDelta>
  bool checkNeighborEntries(
      const NeighborEntryDelta& neighborEntryDelta,
      bool checkEncapIndex = true) const {
    const auto& oldEntry = neighborEntryDelta.getOld();
    const auto& newEntry = neighborEntryDelta.getNew();
    if (!newEntry || newEntry->isPending()) {
      // New entry must be reachable
      return false;
    }
    if (checkEncapIndex && !newHasEncapIndx(neighborEntryDelta)) {
      return false;
    }
    if (checkEncapIndex && oldHasEncapIndx(neighborEntryDelta)) {
      return false;
    }
    // If there was a old entry it must be pending
    return oldEntry ? oldEntry->isPending() : true;
  }

 public:
  WaitForNeighborEntryReachable(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1))
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);

              return this->checkNeighborEntries(neighborEntryDelta);
            },
            "WaitForNeighborEntryReachable@" + ipAddress.str()) {}
  WaitForNeighborEntryReachable(
      SwSwitch* sw,
      IPTYPE ipAddress,
      InterfaceID intf)
      : WaitForSwitchState(
            sw,
            [ipAddress, intf, this](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, intf);

              return this->checkNeighborEntries(neighborEntryDelta);
            },
            "WaitForNeighborEntryReachable@" + ipAddress.str()) {}
  ~WaitForNeighborEntryReachable() {}
};

typedef WaitForNeighborEntryExpiration<IPAddressV4> WaitForArpEntryExpiration;
typedef WaitForNeighborEntryCreation<IPAddressV4> WaitForArpEntryCreation;
typedef WaitForNeighborEntryPending<IPAddressV4> WaitForArpEntryPending;
typedef WaitForNeighborEntryReachable<IPAddressV4> WaitForArpEntryReachable;

typedef WaitForNeighborEntryExpiration<IPAddressV6> WaitForNdpEntryExpiration;
typedef WaitForNeighborEntryCreation<IPAddressV6> WaitForNdpEntryCreation;
typedef WaitForNeighborEntryPending<IPAddressV6> WaitForNdpEntryPending;
typedef WaitForNeighborEntryReachable<IPAddressV6> WaitForNdpEntryReachable;

} // namespace facebook::fboss
