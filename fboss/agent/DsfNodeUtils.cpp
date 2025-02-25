/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/DsfNodeUtils.h"

namespace facebook::fboss::utility {
bool isDualStage(const cfg::AgentConfig& cfg) {
  for (auto const& [_, dsfNode] : *cfg.sw()->dsfNodes()) {
    if (dsfNode.fabricLevel().has_value() && dsfNode.fabricLevel() == 2) {
      return true;
    }
  }
  return false;
}

} // namespace facebook::fboss::utility
