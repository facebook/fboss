/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/NodeBase.h"

#include <atomic>

namespace {
std::atomic<uint64_t> nextNodeID;
}

namespace facebook::fboss {

NodeBase::NodeBase()
    : nodeID_(nextNodeID.fetch_add(1, std::memory_order_relaxed)) {}

} // namespace facebook::fboss
