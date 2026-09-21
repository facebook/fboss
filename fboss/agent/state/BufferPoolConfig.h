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

#include <string>
#include <utility>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

USE_THRIFT_COW(BufferPoolCfg)
/*
 * BufferPoolCfg stores the buffer pool related properites as need by {port, PG}
 */
class BufferPoolCfg : public ThriftStructNode<BufferPoolCfg, BufferPoolFields> {
 public:
  using BaseT = ThriftStructNode<BufferPoolCfg, BufferPoolFields>;

  explicit BufferPoolCfg(const std::string& id) {
    set<common_if_tags::id>(id);
  }
  BufferPoolCfg(const std::string& id, int sharedBytes, int headroomBytes) {
    set<common_if_tags::id>(id);
    setSharedBytes(sharedBytes);
    setHeadroomBytes(headroomBytes);
  }
  int getSharedBytes() const {
    return get<common_if_tags::sharedBytes>()->cref();
  }

  std::optional<int> getHeadroomBytes() const {
    if (auto headroomBytes = safe_cref<common_if_tags::headroomBytes>()) {
      return headroomBytes->toThrift();
    }
    return std::nullopt;
  }

  std::optional<int> getReservedBytes() const {
    if (auto reservedBytes = safe_cref<common_if_tags::reservedBytes>()) {
      return reservedBytes->toThrift();
    }
    return std::nullopt;
  }

  const std::string& getID() const {
    return cref<common_if_tags::id>()->cref();
  }

  void setHeadroomBytes(int headroomBytes) {
    set<common_if_tags::headroomBytes>(headroomBytes);
  }

  void setSharedBytes(int sharedBytes) {
    set<common_if_tags::sharedBytes>(sharedBytes);
  }

  void setReservedBytes(std::optional<int> reservedBytes) {
    if (reservedBytes) {
      set<common_if_tags::reservedBytes>(*reservedBytes);
    } else {
      ref<common_if_tags::reservedBytes>().reset();
    }
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using BufferPoolConfigs = std::vector<std::shared_ptr<BufferPoolCfg>>;
} // namespace facebook::fboss
