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

#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/bounce/interface/CmdBounceInterface.h"
#include "fboss/cli/fboss2/commands/clear/CmdClearArp.h"
#include "fboss/cli/fboss2/commands/clear/CmdClearInterfaceCounters.h"
#include "fboss/cli/fboss2/commands/clear/CmdClearNdp.h"
#include "fboss/cli/fboss2/commands/clear/interface/CmdClearInterface.h"
#include "fboss/cli/fboss2/commands/clear/interface/prbs/CmdClearInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/clear/interface/prbs/stats/CmdClearInterfacePrbsStats.h"
#include "fboss/cli/fboss2/commands/show/acl/CmdShowAcl.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/CmdShowAggregatePort.h"
#include "fboss/cli/fboss2/commands/show/arp/CmdShowArp.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/CmdShowInterfaceCountersMKA.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/CmdShowInterfaceErrors.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/CmdShowInterfaceFlaps.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/CmdShowInterfacePhymap.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/CmdShowInterfacePrbsCapabilities.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/CmdShowInterfacePrbsStats.h"
#include "fboss/cli/fboss2/commands/show/interface/status/CmdShowInterfaceStatus.h"
#include "fboss/cli/fboss2/commands/show/interface/traffic/CmdShowInterfaceTraffic.h"
#include "fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h"
#include "fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPortQueue.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/sdk/dump/CmdShowSdkDump.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"

namespace facebook::fboss {

const CommandTree& kCommandTree() {
  const static CommandTree root = {
      {"show",
       "acl",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show ACL information",
       commandHandler<CmdShowAcl>},

      {"show",
       "aggregate-port",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show aggregate port information",
       commandHandler<CmdShowAggregatePort>},

      {"show",
       "arp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show ARP information",
       commandHandler<CmdShowArp>},

      {"show",
       "lldp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show LLDPinformation",
       commandHandler<CmdShowLldp>},

      {"show",
       "ndp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST,
       "Show NDP information",
       commandHandler<CmdShowNdp>},

      {"show",
       "port",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show Port queue information",
       commandHandler<CmdShowPort>,
       {

           {"queue",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
            "Show Port queue information",
            commandHandler<CmdShowPortQueue>}}},

      {"show",
       "interface",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show Interface information",
       commandHandler<CmdShowInterface>,
       {
           {"counters",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
            "Show Interface Counters",
            commandHandler<CmdShowInterfaceCounters>,
            {
                {"mka",
                 utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
                 "Show Interface MKA counters",
                 commandHandler<CmdShowInterfaceCountersMKA>},
            }},

           {"errors",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
            "Show Interface Error Counters",
            commandHandler<CmdShowInterfaceErrors>},

           {"flaps",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
            "Show Interface Flap Counters",
            commandHandler<CmdShowInterfaceFlaps>},

           {"traffic",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
            "Show Interface Traffic",
            commandHandler<CmdShowInterfaceTraffic>},

           {"status",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
            "Show Interface Status",
            commandHandler<CmdShowInterfaceStatus>},
           {"phymap",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
            "Show External Phy Port Map",
            commandHandler<CmdShowInterfacePhymap>},
           {"prbs",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT,
            "Show PRBS information",
            commandHandler<CmdShowInterfacePrbs>,
            {
                {"capabilities",
                 utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
                 "Show PRBS capabilities",
                 commandHandler<CmdShowInterfacePrbsCapabilities>},
                {"stats",
                 utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
                 "Show PRBS stats",
                 commandHandler<CmdShowInterfacePrbsStats>},
            }},
       }},
      {"show",
       "transceiver",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show Transceiver information",
       commandHandler<CmdShowTransceiver>},

      {"show",
       "route",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST,
       "Show route information",
       commandHandler<CmdShowRoute>},

      {
          "show",
          "sdk",
          "Show SDK state",
          {
              {
                  "dump",
                  utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
                  "dump",
                  commandHandler<CmdShowSdkDump>,
              },
          },
      },

      {"clear",
       "arp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Clear ARP information",
       commandHandler<CmdClearArp>},

      {"clear",
       "ndp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Clear NDP information",
       commandHandler<CmdClearNdp>},

      {
          "clear",
          "interface",
          utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
          "Clear Interface Information",
          commandHandler<CmdClearInterface>,
          {
              {
                  "counters",
                  utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
                  "Clear Interface Counters",
                  commandHandler<CmdClearInterfaceCounters>,
              },
              {
                  "prbs",
                  utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT,
                  "Clear PRBS Information",
                  commandHandler<CmdClearInterfacePrbs>,
                  {{"stats",
                    utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
                    "Clear PRBS stats",
                    commandHandler<CmdClearInterfacePrbsStats>}},
              },
          },
      },
      {"bounce",
       "interface",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Shut/No-Shut Interface",
       commandHandler<CmdBounceInterface>},
  };
  return root;
}

} // namespace facebook::fboss
