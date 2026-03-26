// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/CmdList.h"

#include <algorithm>

namespace facebook::fboss {

// Defined in CmdListConfig.cpp (fboss2-config-lib)
const CommandTree& kConfigCommandTree();

const CommandTree& kAdditionalCommandTree() {
  static CommandTree merged = []() {
    CommandTree result = kBaseAdditionalCommandTree();
    const auto& configCommands = kConfigCommandTree();
    result.insert(result.end(), configCommands.begin(), configCommands.end());
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
