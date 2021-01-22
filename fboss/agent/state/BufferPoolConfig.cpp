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

state::BufferPoolFields BufferPoolCfgFields::toThrift() const {
  state::BufferPoolFields bufferPool;

  bufferPool.id_ref() = id;
  bufferPool.sharedBytes_ref() = static_cast<int>(sharedBytes);
  bufferPool.headroomBytes_ref() = static_cast<int>(headroomBytes);
  return bufferPool;
}

BufferPoolCfgFields BufferPoolCfgFields::fromThrift(
    state::BufferPoolFields const& bufferPoolConfig) {
  BufferPoolCfgFields bufferPool;
  bufferPool.id = static_cast<std::string>(*bufferPoolConfig.id_ref());

  bufferPool.sharedBytes = *bufferPoolConfig.sharedBytes_ref();
  bufferPool.headroomBytes = *bufferPoolConfig.headroomBytes_ref();
  return bufferPool;
}

template class NodeBaseT<BufferPoolCfg, BufferPoolCfgFields>;

} // namespace facebook::fboss
