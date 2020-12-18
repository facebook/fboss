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

folly::dynamic BufferPoolCfgFields::toFollyDynamic() const {
  folly::dynamic bufferPool = folly::dynamic::object;

  bufferPool[kSharedBytes] = static_cast<int>(sharedBytes);
  bufferPool[kHeadroomBytes] = static_cast<int>(headroomBytes);
  bufferPool[kBufferPoolCfgName] = id;
  return bufferPool;
}

BufferPoolCfgFields BufferPoolCfgFields::fromFollyDynamic(
    const folly::dynamic& bufferPoolConfigJson) {
  auto id = bufferPoolConfigJson[kBufferPoolCfgName].asString();
  int sharedBytes = bufferPoolConfigJson[kSharedBytes].asInt();
  int headroomBytes = bufferPoolConfigJson[kHeadroomBytes].asInt();
  return BufferPoolCfgFields(id, sharedBytes, headroomBytes);
}

BufferPoolCfg::BufferPoolCfg(
    const std::string& id,
    const int sharedBytes,
    const int headroomBytes)
    : NodeBaseT(id, sharedBytes, headroomBytes) {}

template class NodeBaseT<BufferPoolCfg, BufferPoolCfgFields>;

} // namespace facebook::fboss
