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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

USE_THRIFT_COW(BufferPoolCfg)
/*
 * BufferPoolCfg stores the buffer pool related properites as need by {port, PG}
 */
class BufferPoolCfg
    : public ThriftStructNode<BufferPoolCfg, state::BufferPoolFields> {
 public:
  using BaseT = ThriftStructNode<BufferPoolCfg, state::BufferPoolFields>;

  explicit BufferPoolCfg(const std::string& id) {
    set<switch_state_tags::id>(id);
  }
  BufferPoolCfg(const std::string& id, int sharedBytes, int headroomBytes) {
    set<switch_state_tags::id>(id);
    setSharedBytes(sharedBytes);
    setHeadroomBytes(headroomBytes);
  }
  int getSharedBytes() const {
    return get<switch_state_tags::sharedBytes>()->cref();
  }

  int getHeadroomBytes() const {
    return get<switch_state_tags::headroomBytes>()->cref();
  }

  const std::string& getID() const {
    return cref<switch_state_tags::id>()->cref();
  }

  void setHeadroomBytes(int headroomBytes) {
    set<switch_state_tags::headroomBytes>(headroomBytes);
  }

  void setSharedBytes(int sharedBytes) {
    set<switch_state_tags::sharedBytes>(sharedBytes);
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using BufferPoolConfigs = std::vector<std::shared_ptr<BufferPoolCfg>>;
} // namespace facebook::fboss
