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
#include "fboss/agent/state/NeighborResponseEntry.h"

namespace facebook::fboss {

class NdpResponseEntryThrifty : public NeighborResponseEntryThrifty<
                                    folly::IPAddressV6,
                                    NdpResponseEntryThrifty> {
 public:
  using NeighborResponseEntryThrifty::NeighborResponseEntryThrifty;
};

} // namespace facebook::fboss
