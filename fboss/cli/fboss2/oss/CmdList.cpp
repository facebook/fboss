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

namespace facebook::fboss {

const std::vector<std::tuple<
    CmdVerb,
    CmdObject,
    utils::ObjectArgTypeId,
    CmdSubCmd,
    CmdHelpMsg,
    CommandHandlerFn>>&
kListOfAdditionalCommands() {
  static const std::vector<std::tuple<
      CmdVerb,
      CmdObject,
      utils::ObjectArgTypeId,
      CmdSubCmd,
      CmdHelpMsg,
      CommandHandlerFn>>
      listOfAdditionalCommands;

  return listOfAdditionalCommands;
}

} // namespace facebook::fboss
