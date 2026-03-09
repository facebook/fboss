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

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigHistoryAgent.h"
#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigRunningAgent.h"
#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigTraits.h"

namespace facebook::fboss {

const CommandTree& kAdditionalCommandTree() {
  static CommandTree root = {
      {"show",
       "config",
       "Show config info for various binaries",
       {{"running",
         "Show running config for various binaries",
         {
             {"agent",
              "Show Agent running config",
              commandHandler<CmdShowConfigRunningAgent>,
              argTypeHandler<CmdShowConfigDynamicTraits>},
         }},
        {"history",
         "Show history of changes to the current config",
         {{"agent",
           "Show history of changes to the current Agent config",
           commandHandler<CmdShowConfigHistoryAgent>,
           argTypeHandler<CmdShowConfigTraits>}}}}},
  };
  sort(root.begin(), root.end());
  return root;
}

const std::vector<Command>& kSpecialCommands() {
  static const std::vector<Command> cmds = {};
  return cmds;
}
} // namespace facebook::fboss
