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

#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using BufferPoolCfgMapTraits = NodeMapTraits<std::string, BufferPoolCfg>;

/*
 * A container for the set of collectors.
 */
class BufferPoolCfgMap
    : public NodeMapT<BufferPoolCfgMap, BufferPoolCfgMapTraits> {
 public:
  BufferPoolCfgMap() = default;
  ~BufferPoolCfgMap() override = default;

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
