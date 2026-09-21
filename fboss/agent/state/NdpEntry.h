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

#include <folly/IPAddressV6.h>
#include "fboss/agent/state/NeighborEntry.h"

namespace facebook::fboss {

USE_THRIFT_COW(NdpEntry);

/*
 * NdpEntry represents an entry in our IPv6 neighbor table.
 *
 * Note that we define NdpEntry as its own class here rather than using a
 * typedef purely to make it easier for other classes to forward declare
 * NdpEntry.
 */
class NdpEntry : public NeighborEntry<folly::IPAddressV6, NdpEntry> {
 public:
  using Base = NeighborEntry<folly::IPAddressV6, NdpEntry>;
  using Base::Base;
};

} // namespace facebook::fboss
