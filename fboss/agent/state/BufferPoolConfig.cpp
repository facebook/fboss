/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/NodeBase-defs.h"

namespace {
constexpr auto kSharedBytes = "sharedBytes";
constexpr auto kHeadroomBytes = "headroomBytes";
constexpr auto kBufferPoolCfgName = "id";
} // namespace

namespace facebook::fboss {

template class ThriftStructNode<BufferPoolCfg, state::BufferPoolFields>;

} // namespace facebook::fboss
