/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FsdbSyncer.h"

namespace facebook::fboss {

std::vector<std::string> FsdbSyncer::getAgentStatePath() {
  return {"agent"};
}

std::vector<std::string> FsdbSyncer::getAgentSwitchStatePath() {
  return {"agent", "switchState"};
}

std::vector<std::string> FsdbSyncer::getAgentSwitchConfigPath() {
  return {"agent", "config", "sw"};
}

std::vector<std::string> FsdbSyncer::getAgentStatsPath() {
  return {"agent"};
}
} // namespace facebook::fboss
