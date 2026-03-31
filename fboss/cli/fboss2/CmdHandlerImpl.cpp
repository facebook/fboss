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
#include "fboss/cli/fboss2/commands/clear/interface/counters/phy/CmdClearInterfaceCountersPhy.h"
#include "fboss/cli/fboss2/commands/clear/interface/prbs/CmdClearInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/clear/interface/prbs/stats/CmdClearInterfacePrbsStats.h"
#include "fboss/cli/fboss2/commands/get/pcap/CmdGetPcap.h"
#include "fboss/cli/fboss2/commands/set/interface/CmdSetInterface.h"
#include "fboss/cli/fboss2/commands/set/interface/prbs/CmdSetInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/set/interface/prbs/state/CmdSetInterfacePrbsState.h"
#include "fboss/cli/fboss2/commands/set/port/CmdSetPort.h"
#include "fboss/cli/fboss2/commands/set/port/state/CmdSetPortState.h"
#include "fboss/cli/fboss2/commands/show/dsf/CmdShowDsf.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/CmdShowInterfaceCountersFec.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/ber/CmdShowInterfaceCountersFecBer.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/ber/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/histogram/CmdShowInterfaceCountersFecHistogram.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/histogram/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/tail/CmdShowInterfaceCountersFecTail.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/tail/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/uncorrectable/CmdShowInterfaceCountersFecUncorrectable.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/uncorrectable/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/CmdShowInterfaceCountersMKA.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/CmdShowInterfaceFlaps.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/CmdShowInterfacePhymap.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/CmdShowInterfacePrbsCapabilities.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/CmdShowInterfacePrbsState.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/CmdShowInterfacePrbsStats.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/start/pcap/CmdStartPcap.h"
#include "fboss/cli/fboss2/commands/stop/pcap/CmdStopPcap.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/fboss_types.h"
#include "fboss/agent/if/gen-cpp2/fboss_visitation.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_visitation.h"
#include "fboss/lib/phy/gen-cpp2/prbs_visitation.h"

namespace facebook::fboss {

// Avoid template linker error
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void CmdHandler<CmdGetPcap, CmdGetPcapTraits>::run();
template void CmdHandler<CmdShowDsf, CmdShowDsfTraits>::run();
template void CmdHandler<
    CmdShowInterfaceCountersFec,
    CmdShowInterfaceCountersFecTraits>::run();
template void CmdHandler<
    CmdShowInterfaceCountersFecBer,
    CmdShowInterfaceCountersFecBerTraits>::run();
template void CmdHandler<
    CmdShowInterfaceCountersFecTail,
    CmdShowInterfaceCountersFecTailTraits>::run();
template void CmdHandler<
    CmdShowInterfaceCountersFecUncorrectable,
    CmdShowInterfaceCountersFecUncorrectableTraits>::run();
template void CmdHandler<
    CmdShowInterfaceCountersFecHistogram,
    CmdShowInterfaceCountersFecHistogramTraits>::run();
template void CmdHandler<
    CmdShowInterfaceCountersMKA,
    CmdShowInterfaceCountersMKATraits>::run();
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
template void
CmdHandler<CmdShowInterfacePhymap, CmdShowInterfacePhymapTraits>::run();
template void CmdHandler<CmdClearInterface, CmdClearInterfaceTraits>::run();
template void CmdHandler<
    CmdClearInterfaceCountersPhy,
    CmdClearInterfaceCountersPhyTraits>::run();
template void
CmdHandler<CmdClearInterfacePrbs, CmdClearInterfacePrbsTraits>::run();
template void
CmdHandler<CmdClearInterfacePrbsStats, CmdClearInterfacePrbsStatsTraits>::run();
template void CmdHandler<CmdClearArp, CmdClearArpTraits>::run();
template void CmdHandler<CmdClearNdp, CmdClearNdpTraits>::run();
template void
CmdHandler<CmdClearInterfaceCounters, CmdClearInterfaceCountersTraits>::run();
template void CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits>::run();
template void CmdHandler<CmdSetPort, CmdSetPortTraits>::run();
template void CmdHandler<CmdSetPortState, CmdSetPortStateTraits>::run();
template void CmdHandler<CmdSetInterface, CmdSetInterfaceTraits>::run();
template void CmdHandler<CmdSetInterfacePrbs, CmdSetInterfacePrbsTraits>::run();
template void
CmdHandler<CmdSetInterfacePrbsState, CmdSetInterfacePrbsStateTraits>::run();
template void CmdHandler<CmdStartPcap, CmdStartPcapTraits>::run();
template void CmdHandler<CmdStopPcap, CmdStopPcapTraits>::run();

template const ValidFilterMapType
CmdHandler<CmdShowDsf, CmdShowDsfTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFec,
    CmdShowInterfaceCountersFecTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFecBer,
    CmdShowInterfaceCountersFecBerTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFecTail,
    CmdShowInterfaceCountersFecTailTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFecUncorrectable,
    CmdShowInterfaceCountersFecUncorrectableTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFecHistogram,
    CmdShowInterfaceCountersFecHistogramTraits>::getValidFilters();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersMKA,
    CmdShowInterfaceCountersMKATraits>::getValidFilters();
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
template const ValidFilterMapType CmdHandler<
    CmdShowInterfacePhymap,
    CmdShowInterfacePhymapTraits>::getValidFilters();
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
template const ValidFilterMapType
CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits>::getValidFilters();
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
