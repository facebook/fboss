/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/NdpEntry.h"

#include "fboss/agent/state/NeighborEntry-defs.h"

namespace facebook::fboss {

template class NeighborEntry<folly::IPAddressV6, NdpEntry>;

} // namespace facebook::fboss
