// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/CmdList.h"

namespace facebook::fboss {

const CommandTree& kAdditionalCommandTree() {
  return kBaseAdditionalCommandTree();
}

const std::vector<Command>& kSpecialCommands() {
  static const std::vector<Command> cmds = {};
  return cmds;
}

} // namespace facebook::fboss
