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
#include "fboss/agent/state/UdfGroupMap.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class UdfConfig;
USE_THRIFT_COW(UdfConfig);
RESOLVE_STRUCT_MEMBER(UdfConfig, switch_config_tags::udfGroups, UdfGroupMap);

class UdfConfig : public ThriftStructNode<UdfConfig, cfg::UdfConfig> {
 public:
  using BaseT = ThriftStructNode<UdfConfig, cfg::UdfConfig>;
  explicit UdfConfig() {}

  std::shared_ptr<UdfGroupMap> getUdfGroupMap() const {
    return get<switch_config_tags::udfGroups>();
  }

  bool isUdfConfigPopulated() const {
    // check if udf state object is populated or not
    if (getUdfGroupMap() && getUdfGroupMap()->size()) {
      return true;
    }
    return false;
  }

  static std::shared_ptr<UdfConfig> fromFollyDynamic(
      const folly::dynamic& group);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
