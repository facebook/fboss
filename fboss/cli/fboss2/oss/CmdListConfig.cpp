/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdList.h"

#include "fboss/cli/fboss2/oss/CmdListShowConfig.h"

namespace facebook::fboss {

// Defined in CmdListConfig.cpp
const CommandTree& kConfigCommandTree();

const CommandTree& kAdditionalCommandTree() {
  // Merge config commands with show config commands
  static CommandTree merged = []() {
    CommandTree result = kConfigCommandTree();
    CommandTree showConfigCommands = kShowConfigCommandTree();
    result.insert(
        result.end(), showConfigCommands.begin(), showConfigCommands.end());
    sort(result.begin(), result.end());
    return result;
  }();

  return merged;
}

const std::vector<Command>& kSpecialCommands() {
  static const std::vector<Command> cmds = {};
  return cmds;
}

} // namespace facebook::fboss
