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
  using NeighborEntriesDelta = MapDelta<state::NeighborEntries>;
  using DeltaValue<Interface>::DeltaValue;
  using ArpTableDelta = ThriftMapDelta<ArpTable>;
  using NdpTableDelta = ThriftMapDelta<NdpTable>;

  ArpTableDelta getArpDelta() const {
    return ArpTableDelta(
        getOld() ? getOld()->getArpTable().get() : nullptr,
        getNew() ? getNew()->getArpTable().get() : nullptr);
  }
  NdpTableDelta getNdpDelta() const {
    return NdpTableDelta(
        getOld() ? getOld()->getNdpTable().get() : nullptr,
        getNew() ? getNew()->getNdpTable().get() : nullptr);
  }

  template <typename NTableT>
  ThriftMapDelta<NTableT> getNeighborDelta() const;

 private:
  auto* getArpEntries(const std::shared_ptr<Interface>& intf) const {
    return intf ? intf->getArpTable().get() : nullptr;
  }
  auto* getNdpEntries(const std::shared_ptr<Interface>& intf) const {
    return intf ? intf->getNdpTable().get() : nullptr;
  }

 public:
  auto getArpEntriesDelta() const {
    return ThriftMapDelta(getArpEntries(getOld()), getArpEntries(getNew()));
  }
  auto getNdpEntriesDelta() const {
    return ThriftMapDelta(getNdpEntries(getOld()), getNdpEntries(getNew()));
  }
};

template <>
inline ThriftMapDelta<ArpTable> InterfaceDelta::getNeighborDelta() const {
  return getArpDelta();
}

template <>
inline ThriftMapDelta<NdpTable> InterfaceDelta::getNeighborDelta() const {
  return getNdpDelta();
}

template <typename IGNORED>
struct InterfaceMapDeltaTraits {
  using mapped_type = typename InterfaceMap::mapped_type;
  using Extractor = ExtractorT<InterfaceMap>;
  using Delta = InterfaceDelta;
  using NodeWrapper = typename Delta::NodeWrapper;
  using DeltaValueIterator =
      DeltaValueIteratorT<InterfaceMap, Delta, Extractor>;
  using MapPointerTraits = MapPointerTraitsT<InterfaceMap>;
};

using InterfaceMapDelta = MapDelta<InterfaceMap, InterfaceMapDeltaTraits>;

using MultiSwitchInterafceMapDeltaTraits = NestedMapDeltaTraits<
    MultiSwitchInterfaceMap,
    InterfaceMap,
    ThriftMapDelta,
    MapDelta,
    MapDeltaTraits,
    InterfaceMapDeltaTraits>;

using MultiSwitchInterfaceMapDelta =
    NestedMapDelta<MultiSwitchInterafceMapDeltaTraits>;
} // namespace facebook::fboss
