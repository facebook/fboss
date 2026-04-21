// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/CmdList.h"

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigHistoryAgent.h"
#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigRunningAgent.h"
#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigTraits.h"

namespace facebook::fboss {

const CommandTree& kBaseAdditionalCommandTree() {
  static CommandTree tree = {
      {"show",
       "config",
       "Show config info for various binaries",
       {{"running",
         "Show running config for various binaries",
         {
             {"agent",
              "Show Agent running config",
              commandHandler<CmdShowConfigRunningAgent>,
              argRegistrar<CmdShowConfigDynamicTraits>},
         }},
        {"history",
         "Show history of changes to the current config",
         {{"agent",
           "Show history of changes to the current Agent config",
           commandHandler<CmdShowConfigHistoryAgent>,
           argRegistrar<CmdShowConfigTraits>}}}}},
  };
  return tree;
}

} // namespace facebook::fboss
