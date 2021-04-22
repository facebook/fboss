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
#include "fboss/cli/fboss2/CmdShowPort.h"
#include "fboss/cli/fboss2/CmdShowPortQueue.h"

namespace facebook::fboss {

const std::vector<std::tuple<
    std::string,
    std::string,
    utils::ObjectArgTypeId,
    std::string,
    std::string,
    CommandHandlerFn>>&
kListOfCommands() {
  static const std::vector<std::tuple<
      std::string,
      std::string,
      utils::ObjectArgTypeId,
      std::string,
      std::string,
      CommandHandlerFn>>
      listOfCommands = {
          {"show",
           "acl",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
           "",
           "Show ACL information",
           commandHandler<CmdShowAcl>},

          {"show",
           "arp",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
           "",
           "Show ARP information",
           commandHandler<CmdShowArp>},

          {"show",
           "lldp",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
           "",
           "Show LLDPinformation",
           commandHandler<CmdShowLldp>},

          {"show",
           "ndp",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST,
           "",
           "Show NDP information",
           commandHandler<CmdShowNdp>},

          {"show",
           "port",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
           "",
           "Show Port information",
           commandHandler<CmdShowPort>},

          {"show",
           "port",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
           "queue",
           "Show Port queue information",
           commandHandler<CmdShowPortQueue>},

          {"clear",
           "arp",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
           "",
           "Clear ARP information",
           commandHandler<CmdClearArp>},

          {"clear",
           "ndp",
           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
           "",
           "Clear NDP information",
           commandHandler<CmdClearNdp>},
      };

  return listOfCommands;
}

} // namespace facebook::fboss
