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
#include "fboss/agent/state/NeighborResponseEntry.h"

namespace facebook::fboss {

class ArpResponseEntry
    : public NeighborResponseEntry<folly::IPAddressV4, ArpResponseEntry> {
 public:
  using NeighborResponseEntry::NeighborResponseEntry;
};

} // namespace facebook::fboss
