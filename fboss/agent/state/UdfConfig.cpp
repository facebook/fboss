/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/UdfConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_fatal.h"
#include "fboss/agent/gen-cpp2/switch_config_fatal_types.h"

namespace facebook::fboss {

std::shared_ptr<UdfConfig> UdfConfig::fromFollyDynamic(
    const folly::dynamic& entry) {
  auto node = std::make_shared<UdfConfig>();
  static_cast<std::shared_ptr<BaseT>>(node)->fromFollyDynamic(entry);
  return node;
}

template class ThriftStructNode<UdfConfig, cfg::UdfConfig>;
} // namespace facebook::fboss
