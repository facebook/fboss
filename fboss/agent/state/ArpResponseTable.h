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

#include <folly/IPAddressV4.h>
#include "fboss/agent/state/NeighborResponseTable.h"

namespace facebook::fboss {

/*
 * A mapping of IPv4 --> MAC address, indicating how we should respond to ARP
 * requests on a given VLAN.
 *
 * This information is computed from the interface configuration, but is stored
 * with each VLAN so that we can efficiently respond to ARP requests.
 */
class ArpResponseTable
    : public NeighborResponseTable<folly::IPAddressV4, ArpResponseTable> {
 public:
  using NeighborResponseTable::NeighborResponseTable;
};

} // namespace facebook::fboss
