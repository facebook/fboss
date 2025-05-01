/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/inputbalance/InputBalanceUtil.h"

namespace facebook::fboss::utility {

bool isDualStage(const std::map<int64_t, cfg::DsfNode>& dsfNodeMap) {
  for (const auto& [_, dsfNode] : dsfNodeMap) {
    if (dsfNode.fabricLevel().has_value() && dsfNode.fabricLevel() == 2) {
      return true;
    }
  }
  return false;
}

std::vector<std::pair<int64_t, std::string>> deviceToQueryInputCapacity(
    const std::vector<int64_t>& /* fabricSwitchIDs */,
    const std::map<int64_t, cfg::DsfNode>& /* dsfNodeMap */) {
  // TODO(zecheng): Implement this
  return {};
}

} // namespace facebook::fboss::utility
