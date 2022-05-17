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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using BufferPoolCfgMapTraits = NodeMapTraits<std::string, BufferPoolCfg>;

struct BufferPoolCfgMapThriftTraits
    : public ThriftyNodeMapTraits<std::string, state::BufferPoolFields> {
  static inline const std::string& getThriftKeyName() {
    static const std::string _key = "id";
    return _key;
  }
  static const KeyType parseKey(const folly::dynamic& key) {
    return key.asString();
  }
};
/*
 * A container for the set of collectors.
 */
class BufferPoolCfgMap : public ThriftyNodeMapT<
                             BufferPoolCfgMap,
                             BufferPoolCfgMapTraits,
                             BufferPoolCfgMapThriftTraits> {
 public:
  BufferPoolCfgMap() = default;
  ~BufferPoolCfgMap() override = default;

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
