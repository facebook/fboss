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
#include "fboss/agent/state/MapDelta.h"
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
  using ArpTableDelta = NodeMapDelta<ArpTable>;
  using NdpTableDelta = NodeMapDelta<NdpTable>;
  using NeighborEntriesDelta = MapDelta<state::NeighborEntries>;

  using DeltaValue<Interface>::DeltaValue;

  ArpTableDelta getArpDelta() const {
    oldArpTable_ = oldArpTable_ ? oldArpTable_ : getArpTable(getOld());
    newArpTable_ = newArpTable_ ? newArpTable_ : getArpTable(getNew());
    return ArpTableDelta(oldArpTable_.get(), newArpTable_.get());
  }
  NdpTableDelta getNdpDelta() const {
    oldNdpTable_ = oldNdpTable_ ? oldNdpTable_ : getNdpTable(getOld());
    newNdpTable_ = newNdpTable_ ? newNdpTable_ : getNdpTable(getNew());
    return NdpTableDelta(oldNdpTable_.get(), newNdpTable_.get());
  }
  template <typename NTableT>
  NodeMapDelta<NTableT> getNeighborDelta() const;

  NeighborEntriesDelta getArpEntriesDelta() const {
    return NeighborEntriesDelta(
        getArpEntries(getOld()), getArpEntries(getNew()));
  }
  NeighborEntriesDelta getNdpEntriesDelta() const {
    return NeighborEntriesDelta(
        getNdpEntries(getOld()), getNdpEntries(getNew()));
  }

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
  const state::NeighborEntries* getArpEntries(
      const std::shared_ptr<Interface>& intf) const {
    return intf ? &intf->getArpTable() : nullptr;
  }
  const state::NeighborEntries* getNdpEntries(
      const std::shared_ptr<Interface>& intf) const {
    return intf ? &intf->getNdpTable() : nullptr;
  }

  /*
   * Convert from state::NeighborEntries to Arp, Ndp table to
   * be able to reuse NodeMapDelta. Mutable since we do a
   * lazy conversion for when delta is asked for
   */
  mutable std::shared_ptr<ArpTable> oldArpTable_, newArpTable_;
  mutable std::shared_ptr<NdpTable> oldNdpTable_, newNdpTable_;
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
