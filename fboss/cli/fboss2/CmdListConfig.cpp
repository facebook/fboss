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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/CmdConfigAppliedInfo.h"
#include "fboss/cli/fboss2/commands/config/CmdConfigReload.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionCommit.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"

namespace facebook::fboss {

const CommandTree& kConfigCommandTree() {
  static CommandTree root = {
      {"config",
       "applied-info",
       "Show config applied information",
       commandHandler<CmdConfigAppliedInfo>,
       argTypeHandler<CmdConfigAppliedInfoTraits>},

      {
          "config",
          "session",
          "Manage config session",
          {{
               "commit",
               "Commit the current config session",
               commandHandler<CmdConfigSessionCommit>,
               argTypeHandler<CmdConfigSessionCommitTraits>,
           },
           {
               "diff",
               "Show diff between configs (session vs live, session vs revision, or revision vs revision)",
               commandHandler<CmdConfigSessionDiff>,
               argTypeHandler<CmdConfigSessionDiffTraits>,
           }},
      },

      {"config",
       "reload",
       "Reload agent configuration",
       commandHandler<CmdConfigReload>,
       argTypeHandler<CmdConfigReloadTraits>},
  };
  sort(root.begin(), root.end());
  return root;
}

} // namespace facebook::fboss
