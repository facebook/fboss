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
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

namespace facebook { namespace fboss {

/*
 * VlanMapDelta is a small wrapper on top of NodeMapDelta<VlanMap>.
 *
 * The main difference is that it's iterator also provides a getArpDelta()
 * helper function.
 */
class VlanDelta : public DeltaValue<Vlan> {
 public:
  typedef NodeMapDelta<ArpTable> ArpTableDelta;
  typedef NodeMapDelta<NdpTable> NdpTableDelta;

  using DeltaValue<Vlan>::DeltaValue;

  ArpTableDelta getArpDelta() const {
    return ArpTableDelta(getOld() ? getOld()->getArpTable().get() : nullptr,
                         getNew() ? getNew()->getArpTable().get() : nullptr);
  }
  NdpTableDelta getNdpDelta() const {
    return NdpTableDelta(getOld() ? getOld()->getNdpTable().get() : nullptr,
                         getNew() ? getNew()->getNdpTable().get() : nullptr);
  }
};

typedef NodeMapDelta<VlanMap, VlanDelta> VlanMapDelta;

}} // facebook::fboss
