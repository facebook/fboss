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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

USE_THRIFT_COW(UdfGroup);

class UdfGroup : public ThriftStructNode<UdfGroup, cfg::UdfGroup> {
 public:
  using BaseT = ThriftStructNode<UdfGroup, cfg::UdfGroup>;
  explicit UdfGroup(const std::string& name);

  std::string getID() const {
    return getName();
  }
  std::string getName() const;
  cfg::UdfBaseHeaderType getUdfBaseHeader() const;
  int getStartOffsetInBytes() const;
  int getFieldSizeInBytes() const;
  std::vector<std::string> getUdfPacketMatcherIds() const;
  static std::shared_ptr<UdfGroup> fromFollyDynamic(
      const folly::dynamic& group);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
