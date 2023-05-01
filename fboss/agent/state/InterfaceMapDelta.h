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

template <typename IGNORED>
struct InterfaceMapDeltaTraits {
  using mapped_type = typename InterfaceMap::mapped_type;
  using ExtractorT = ThriftMapNodeExtractor<InterfaceMap>;
  using DeltaValueT = InterfaceDelta;
};

using InterfaceMapDelta = MapDelta<InterfaceMap, InterfaceMapDeltaTraits>;

} // namespace facebook::fboss
