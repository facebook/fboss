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

namespace facebook::fboss {

void commandHandler() {
  CmdHandler::getInstance()->run();
}

const std::vector<
    std::tuple<std::string, std::string, std::string, CommandHandlerFn>>&
kListOfCommands() {
  static const std::vector<
      std::tuple<std::string, std::string, std::string, CommandHandlerFn>>
      listOfCommands = {
          {"show", "acl", "Show ACL information", commandHandler}};

  return listOfCommands;
}

} // namespace facebook::fboss
