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

#include "fboss/cli/fboss2/CmdClearArp.h"
#include "fboss/cli/fboss2/CmdClearNdp.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdShowAcl.h"
#include "fboss/cli/fboss2/CmdShowArp.h"
#include "fboss/cli/fboss2/CmdShowLldp.h"
#include "fboss/cli/fboss2/CmdShowNdp.h"

namespace facebook::fboss {

const std::vector<
    std::tuple<std::string, std::string, std::string, CommandHandlerFn>>&
kListOfCommands() {
  static const std::vector<
      std::tuple<std::string, std::string, std::string, CommandHandlerFn>>
      listOfCommands = {
          {"show", "acl", "Show ACL information", commandHandler<CmdShowAcl>},
          {"show", "arp", "Show ARP information", commandHandler<CmdShowArp>},
          {"show", "lldp", "Show LLDPinformation", commandHandler<CmdShowLldp>},
          {"show", "ndp", "Show NDP information", commandHandler<CmdShowNdp>},

          {"clear", "arp", "Clear ARP information", commandHandler<CmdClearArp>},
          {"clear", "ndp", "Clear NDP information", commandHandler<CmdClearNdp>},
      };

  return listOfCommands;
}

} // namespace facebook::fboss
