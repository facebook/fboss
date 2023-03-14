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
#include "fboss/cli/fboss2/commands/help/CmdHelp.h"
#include "fboss/cli/fboss2/commands/set/interface/CmdSetInterface.h"
#include "fboss/cli/fboss2/commands/set/interface/prbs/CmdSetInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/set/interface/prbs/state/CmdSetInterfacePrbsState.h"
#include "fboss/cli/fboss2/commands/set/port/CmdSetPort.h"
#include "fboss/cli/fboss2/commands/set/port/state/CmdSetPortState.h"
#include "fboss/cli/fboss2/commands/show/acl/CmdShowAcl.h"
#include "fboss/cli/fboss2/commands/show/agent/CmdShowAgentSsl.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/CmdShowAggregatePort.h"
#include "fboss/cli/fboss2/commands/show/arp/CmdShowArp.h"
#include "fboss/cli/fboss2/commands/show/dsfnodes/CmdShowDsfNodes.h"
#include "fboss/cli/fboss2/commands/show/fabric/CmdShowFabric.h"
#include "fboss/cli/fboss2/commands/show/host/CmdShowHost.h"
#include "fboss/cli/fboss2/commands/show/hwobject/CmdShowHwObject.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/CmdShowInterfaceCountersMKA.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/CmdShowInterfaceErrors.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/CmdShowInterfaceFlaps.h"
#include "fboss/cli/fboss2/commands/show/interface/phy/CmdShowInterfacePhy.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/CmdShowInterfacePhymap.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/CmdShowInterfacePrbsCapabilities.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/CmdShowInterfacePrbsState.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/CmdShowInterfacePrbsStats.h"
#include "fboss/cli/fboss2/commands/show/interface/status/CmdShowInterfaceStatus.h"
#include "fboss/cli/fboss2/commands/show/interface/traffic/CmdShowInterfaceTraffic.h"
#include "fboss/cli/fboss2/commands/show/l2/CmdShowL2.h"
#include "fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h"
#include "fboss/cli/fboss2/commands/show/mac/CmdShowMacAddrToBlock.h"
#include "fboss/cli/fboss2/commands/show/mac/CmdShowMacDetails.h"
#include "fboss/cli/fboss2/commands/show/mirror/CmdShowMirror.h"
#include "fboss/cli/fboss2/commands/show/mpls/CmdShowMplsRoute.h"
#include "fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPortQueue.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteDetails.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteSummary.h"
#include "fboss/cli/fboss2/commands/show/sdk/dump/CmdShowSdkDump.h"
#include "fboss/cli/fboss2/commands/show/systemport/CmdShowSystemPort.h"
#include "fboss/cli/fboss2/commands/show/teflow/CmdShowTeFlow.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"

namespace facebook::fboss {

const CommandTree& kCommandTree() {
  const static CommandTree root = {
      {"show",
       "acl",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show ACL information",
       commandHandler<CmdShowAcl>,
       getValidFilterHandler<CmdShowAcl>},

      {"show",
       "agent",
       "Show agent state",
       {{"ssl",
         utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
         "Show Agent SSL information",
         commandHandler<CmdShowAgentSsl>}}},

      {"show",
       "aggregate-port",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show aggregate port information",
       commandHandler<CmdShowAggregatePort>,
       getValidFilterHandler<CmdShowAggregatePort>},

      {"show",
       "arp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show ARP information",
       commandHandler<CmdShowArp>,
       getValidFilterHandler<CmdShowArp>},

      {"show",
       "fabric",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show Fabric reachability",
       commandHandler<CmdShowFabric>},

      {"show",
       "dsfnodes",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show DsfNodes cluster",
       commandHandler<CmdShowDsfNodes>},

      {"show",
       "lldp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show LLDPinformation",
       commandHandler<CmdShowLldp>},

      {"show",
       "ndp",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST,
       "Show NDP information",
       commandHandler<CmdShowNdp>,
       getValidFilterHandler<CmdShowNdp>},

      {"show",
       "port",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Show Port queue information",
       commandHandler<CmdShowPort>,
       getValidFilterHandler<CmdShowPort>,
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
           {"phy",
            utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PHY_CHIP_TYPE,
            "Show Phy info for asic and/or xphy",
            commandHandler<CmdShowInterfacePhy>},
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
                {"state",
                 utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
                 "Show PRBS state",
                 commandHandler<CmdShowInterfacePrbsState>},
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
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show route information",
       commandHandler<CmdShowRoute>,
       {{"details",
         utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST,
         "Show details of the route table",
         commandHandler<CmdShowRouteDetails>},
        {"summary",
         utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
         "Print a summary of routing tables",
         commandHandler<CmdShowRouteSummary>}}},

      {"show",
       "mac",
       "Show mac information",
       {{"blocked",
         utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
         "Show details of blocked Mac addresses ",
         commandHandler<CmdShowMacAddrToBlock>},
        {"details",
         utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
         "Show details of the Mac(L2) table ",
         commandHandler<CmdShowMacDetails>}}},

      {"show",
       "mirror",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show mirror",
       commandHandler<CmdShowMirror>},

      {"show",
       "mpls",
       "Show mpls information",
       {{"route",
         utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
         "Show details of mpls routes",
         commandHandler<CmdShowMplsRoute>}}},

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

      {"show",
       "systemport",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_SYSTEM_PORT_LIST,
       "Show system port information",
       commandHandler<CmdShowSystemPort>,
       getValidFilterHandler<CmdShowSystemPort>},

      {"show",
       "teflow",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST,
       "Show teflow information",
       commandHandler<CmdShowTeFlow>},

      {"show",
       "host",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show Host Information",
       commandHandler<CmdShowHost>},

      {"show",
       "hw-object",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_HW_OBJECT_LIST,
       "Show HW Objects",
       commandHandler<CmdShowHwObject>},

      {"show",
       "l2",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE,
       "Show L2 Packet Information",
       commandHandler<CmdShowL2>},

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
      {
          "set",
          "interface",
          utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
          "Set Interface information",
          commandHandler<CmdSetInterface>,
          {{
              "prbs",
              utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT,
              "Set PRBS properties",
              commandHandler<CmdSetInterfacePrbs>,
              {{"state",
                utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_STATE,
                "Set PRBS state",
                commandHandler<CmdSetInterfacePrbsState>}},
          }},
      },
      {"set",
       "port",
       utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST,
       "Set Port information",
       commandHandler<CmdSetPort>,
       {{"state",
         utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PORT_STATE,
         "Set Port state",
         commandHandler<CmdSetPortState>}}},
  };
  return root;
}

void helpHandler() {
  auto cmdTree = kCommandTree();
  auto addCmdTree = kAdditionalCommandTree();
  std::vector<CommandTree> cmdTrees = {cmdTree, addCmdTree};
  CmdHelp(cmdTrees).run();
}

} // namespace facebook::fboss
