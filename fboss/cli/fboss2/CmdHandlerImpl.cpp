/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdHandler.cpp"

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
#include "fboss/cli/fboss2/commands/show/acl/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/agent/CmdShowAgentSsl.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/CmdShowAggregatePort.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/arp/CmdShowArp.h"
#include "fboss/cli/fboss2/commands/show/arp/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/dsfnodes/CmdShowDsfNodes.h"
#include "fboss/cli/fboss2/commands/show/dsfnodes/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/fabric/CmdShowFabric.h"
#include "fboss/cli/fboss2/commands/show/fabric/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/host/CmdShowHost.h"
#include "fboss/cli/fboss2/commands/show/hwobject/CmdShowHwObject.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/CmdShowInterfaceCountersMKA.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/CmdShowInterfaceErrors.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/CmdShowInterfaceFlaps.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/phy/CmdShowInterfacePhy.h"
#include "fboss/cli/fboss2/commands/show/interface/phy/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/CmdShowInterfacePhymap.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/CmdShowInterfacePrbsCapabilities.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/CmdShowInterfacePrbsState.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/CmdShowInterfacePrbsStats.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/status/CmdShowInterfaceStatus.h"
#include "fboss/cli/fboss2/commands/show/interface/status/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/traffic/CmdShowInterfaceTraffic.h"
#include "fboss/cli/fboss2/commands/show/interface/traffic/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/l2/CmdShowL2.h"
#include "fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h"
#include "fboss/cli/fboss2/commands/show/lldp/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/mac/CmdShowMacAddrToBlock.h"
#include "fboss/cli/fboss2/commands/show/mac/CmdShowMacDetails.h"
#include "fboss/cli/fboss2/commands/show/mac/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/mirror/CmdShowMirror.h"
#include "fboss/cli/fboss2/commands/show/mpls/CmdShowMplsRoute.h"
#include "fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h"
#include "fboss/cli/fboss2/commands/show/ndp/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPortQueue.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteDetails.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteSummary.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/sdk/dump/CmdShowSdkDump.h"
#include "fboss/cli/fboss2/commands/show/systemport/CmdShowSystemPort.h"
#include "fboss/cli/fboss2/commands/show/systemport/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/teflow/CmdShowTeFlow.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"
#include "fboss/cli/fboss2/commands/show/transceiver/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/start/pcap/CmdStartPcap.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/fboss_types.h"
#include "fboss/agent/if/gen-cpp2/fboss_visitation.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_visitation.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_visitation.h"
#include "fboss/lib/phy/gen-cpp2/prbs_visitation.h"

namespace facebook::fboss {

// Avoid template linker error
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void CmdHandler<CmdShowAcl, CmdShowAclTraits>::run();
template void CmdHandler<CmdShowAgentSsl, CmdShowAgentSslTraits>::run();
template void
CmdHandler<CmdShowAggregatePort, CmdShowAggregatePortTraits>::run();
template void CmdHandler<CmdShowArp, CmdShowArpTraits>::run();
template void CmdHandler<CmdShowFabric, CmdShowFabricTraits>::run();
template void CmdHandler<CmdShowDsfNodes, CmdShowDsfNodesTraits>::run();
template void CmdHandler<CmdShowL2, CmdShowL2Traits>::run();
template void CmdHandler<CmdShowLldp, CmdShowLldpTraits>::run();
template void
CmdHandler<CmdShowMacAddrToBlock, CmdShowMacAddrToBlockTraits>::run();
template void CmdHandler<CmdShowMacDetails, CmdShowMacDetailsTraits>::run();
template void CmdHandler<CmdShowMirror, CmdShowMirrorTraits>::run();
template void CmdHandler<CmdShowNdp, CmdShowNdpTraits>::run();
template void CmdHandler<CmdShowPort, CmdShowPortTraits>::run();
template void CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits>::run();
template void CmdHandler<CmdShowHost, CmdShowHostTraits>::run();
template void CmdHandler<CmdShowHwObject, CmdShowHwObjectTraits>::run();
template void CmdHandler<CmdShowInterface, CmdShowInterfaceTraits>::run();
template void
CmdHandler<CmdShowInterfaceCounters, CmdShowInterfaceCountersTraits>::run();
template void CmdHandler<
    CmdShowInterfaceCountersMKA,
    CmdShowInterfaceCountersMKATraits>::run();
template void
CmdHandler<CmdShowInterfaceErrors, CmdShowInterfaceErrorsTraits>::run();
template void
CmdHandler<CmdShowInterfaceFlaps, CmdShowInterfaceFlapsTraits>::run();
template void
CmdHandler<CmdShowInterfacePrbs, CmdShowInterfacePrbsTraits>::run();
template void CmdHandler<
    CmdShowInterfacePrbsCapabilities,
    CmdShowInterfacePrbsCapabilitiesTraits>::run();
template void
CmdHandler<CmdShowInterfacePrbsState, CmdShowInterfacePrbsStateTraits>::run();
template void
CmdHandler<CmdShowInterfacePrbsStats, CmdShowInterfacePrbsStatsTraits>::run();
template void CmdHandler<CmdShowInterfacePhy, CmdShowInterfacePhyTraits>::run();
template void
CmdHandler<CmdShowInterfacePhymap, CmdShowInterfacePhymapTraits>::run();
template void
CmdHandler<CmdShowInterfaceTraffic, CmdShowInterfaceTrafficTraits>::run();
template void CmdHandler<CmdShowSdkDump, CmdShowSdkDumpTraits>::run();
template void CmdHandler<CmdShowSystemPort, CmdShowSystemPortTraits>::run();
template void CmdHandler<CmdShowTransceiver, CmdShowTransceiverTraits>::run();

template void CmdHandler<CmdClearInterface, CmdClearInterfaceTraits>::run();
template void
CmdHandler<CmdClearInterfacePrbs, CmdClearInterfacePrbsTraits>::run();
template void
CmdHandler<CmdClearInterfacePrbsStats, CmdClearInterfacePrbsStatsTraits>::run();
template void CmdHandler<CmdClearArp, CmdClearArpTraits>::run();
template void CmdHandler<CmdClearNdp, CmdClearNdpTraits>::run();
template void
CmdHandler<CmdClearInterfaceCounters, CmdClearInterfaceCountersTraits>::run();
template void
CmdHandler<CmdShowInterfaceStatus, CmdShowInterfaceStatusTraits>::run();
template void CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits>::run();
template void CmdHandler<CmdShowMplsRoute, CmdShowMplsRouteTraits>::run();
template void CmdHandler<CmdShowRoute, CmdShowRouteTraits>::run();
template void CmdHandler<CmdShowRouteDetails, CmdShowRouteDetailsTraits>::run();
template void CmdHandler<CmdShowRouteSummary, CmdShowRouteSummaryTraits>::run();
template void CmdHandler<CmdSetPort, CmdSetPortTraits>::run();
template void CmdHandler<CmdSetPortState, CmdSetPortStateTraits>::run();
template void CmdHandler<CmdSetInterface, CmdSetInterfaceTraits>::run();
template void CmdHandler<CmdSetInterfacePrbs, CmdSetInterfacePrbsTraits>::run();
template void
CmdHandler<CmdSetInterfacePrbsState, CmdSetInterfacePrbsStateTraits>::run();
template void CmdHandler<CmdShowTeFlow, CmdShowTeFlowTraits>::run();
template void CmdHandler<CmdStartPcap, CmdStartPcapTraits>::run();

template const ValidFilterMapType
CmdHandler<CmdShowArp, CmdShowArpTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowFabric, CmdShowFabricTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowDsfNodes, CmdShowDsfNodesTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowLldp, CmdShowLldpTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowMacAddrToBlock,
    CmdShowMacAddrToBlockTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowNdp, CmdShowNdpTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowPort, CmdShowPortTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowInterface, CmdShowInterfaceTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCounters,
    CmdShowInterfaceCountersTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersMKA,
    CmdShowInterfaceCountersMKATraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceErrors,
    CmdShowInterfaceErrorsTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceFlaps,
    CmdShowInterfaceFlapsTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowInterfacePrbs, CmdShowInterfacePrbsTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfacePrbsCapabilities,
    CmdShowInterfacePrbsCapabilitiesTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfacePrbsState,
    CmdShowInterfacePrbsStateTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfacePrbsStats,
    CmdShowInterfacePrbsStatsTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowInterfacePhy, CmdShowInterfacePhyTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfacePhymap,
    CmdShowInterfacePhymapTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceTraffic,
    CmdShowInterfaceTrafficTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowSdkDump, CmdShowSdkDumpTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowSystemPort, CmdShowSystemPortTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowTransceiver, CmdShowTransceiverTraits>::getValidFilters();

template const ValidFilterMapType
CmdHandler<CmdClearInterface, CmdClearInterfaceTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdClearInterfacePrbs,
    CmdClearInterfacePrbsTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdClearInterfacePrbsStats,
    CmdClearInterfacePrbsStatsTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdClearArp, CmdClearArpTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdClearNdp, CmdClearNdpTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdClearInterfaceCounters,
    CmdClearInterfaceCountersTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceStatus,
    CmdShowInterfaceStatusTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowRoute, CmdShowRouteTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowRouteDetails, CmdShowRouteDetailsTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowRouteSummary, CmdShowRouteSummaryTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdSetPort, CmdSetPortTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdSetPortState, CmdSetPortStateTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdSetInterface, CmdSetInterfaceTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdSetInterfacePrbs, CmdSetInterfacePrbsTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdSetInterfacePrbsState,
    CmdSetInterfacePrbsStateTraits>::getValidFilters();

} // namespace facebook::fboss
