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

#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NodeMapDelta.h"

namespace facebook::fboss {

/*
 * InterfaceMapDelta is a small wrapper on top of NodeMapDelta<InterfaceMap>.
 *
 * The main difference is that it's iterator also provides a get{Arp,Ndp}Delta()
 * helper function.
 */
class InterfaceDelta : public DeltaValue<Interface> {
 public:
  typedef NodeMapDelta<ArpTable> ArpTableDelta;
  typedef NodeMapDelta<NdpTable> NdpTableDelta;

  using DeltaValue<Interface>::DeltaValue;

  ArpTableDelta getArpDelta() const {
    auto oldArpTable = getArpTable(getOld());
    auto newArpTable = getArpTable(getNew());
    return ArpTableDelta(oldArpTable.get(), newArpTable.get());
  }
  NdpTableDelta getNdpDelta() const {
    auto oldNdpTable = getNdpTable(getOld());
    auto newNdpTable = getNdpTable(getNew());
    return NdpTableDelta(oldNdpTable.get(), newNdpTable.get());
  }
  template <typename NTableT>
  NodeMapDelta<NTableT> getNeighborDelta() const;

 private:
  std::shared_ptr<ArpTable> getArpTable(
      const std::shared_ptr<Interface>& intf) const {
    return getNeighborTable<ArpTable>(intf);
  }
  std::shared_ptr<NdpTable> getNdpTable(
      const std::shared_ptr<Interface>& intf) const {
    return getNeighborTable<NdpTable>(intf);
  }
  template <typename NTableT>
  std::shared_ptr<NTableT> getNeighborTable(
      const std::shared_ptr<Interface>& intf) const {
    if (!intf) {
      return std::make_shared<NTableT>();
    }
    return NTableT::fromThrift(
        intf->template getNeighborEntryTable<typename NTableT::AddressType>());
  }
};

typedef NodeMapDelta<InterfaceMap, InterfaceDelta> InterfaceMapDelta;

template <>
inline NodeMapDelta<ArpTable> InterfaceDelta::getNeighborDelta() const {
  return getArpDelta();
}

template <>
inline NodeMapDelta<NdpTable> InterfaceDelta::getNeighborDelta() const {
  return getNdpDelta();
}

} // namespace facebook::fboss
