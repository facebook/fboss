/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdHandler.h"

#include <fboss/cli/fboss2/CmdGlobalOptions.h>
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
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/CmdShowInterfacePrbsState.h"
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
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "folly/futures/Future.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include <future>
#include <iostream>

template <typename CmdTypeT>
void printTabular(
    CmdTypeT& cmd,
    std::vector<std::shared_future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    std::ostream& out,
    std::ostream& err) {
  for (auto& result : results) {
    auto [host, data, errStr] = result.get();
    if (results.size() != 1) {
      out << host << "::" << std::endl << std::string(80, '=') << std::endl;
    }

    if (errStr.empty()) {
      cmd.printOutput(data);
    } else {
      err << errStr << std::endl << std::endl;
    }
  }
}

template <typename CmdTypeT>
void printJson(
    const CmdTypeT& /* cmd */,
    std::vector<std::shared_future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    std::ostream& out,
    std::ostream& err) {
  std::map<std::string, typename CmdTypeT::RetType> hostResults;
  for (auto& result : results) {
    auto [host, data, errStr] = result.get();
    if (errStr.empty()) {
      hostResults[host] = data;
    } else {
      err << host << "::" << std::endl << std::string(80, '=') << std::endl;
      err << errStr << std::endl << std::endl;
    }
  }

  out << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
             hostResults)
      << std::endl;
}

namespace facebook::fboss {

// Avoid template linker error
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void CmdHandler<CmdShowAcl, CmdShowAclTraits>::run();
template void
CmdHandler<CmdShowAggregatePort, CmdShowAggregatePortTraits>::run();
template void CmdHandler<CmdShowArp, CmdShowArpTraits>::run();
template void CmdHandler<CmdShowLldp, CmdShowLldpTraits>::run();
template void CmdHandler<CmdShowNdp, CmdShowNdpTraits>::run();
template void CmdHandler<CmdShowPort, CmdShowPortTraits>::run();
template void CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits>::run();
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
template void
CmdHandler<CmdShowInterfacePhymap, CmdShowInterfacePhymapTraits>::run();
template void
CmdHandler<CmdShowInterfaceTraffic, CmdShowInterfaceTrafficTraits>::run();
template void CmdHandler<CmdShowSdkDump, CmdShowSdkDumpTraits>::run();
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
template void CmdHandler<CmdShowRoute, CmdShowRouteTraits>::run();
template void CmdHandler<CmdSetPort, CmdSetPortTraits>::run();
template void CmdHandler<CmdSetPortState, CmdSetPortStateTraits>::run();
template void CmdHandler<CmdSetInterface, CmdSetInterfaceTraits>::run();
template void CmdHandler<CmdSetInterfacePrbs, CmdSetInterfacePrbsTraits>::run();
template void
CmdHandler<CmdSetInterfacePrbsState, CmdSetInterfacePrbsStateTraits>::run();

static bool hasRun = false;

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdHandler<CmdTypeT, CmdTypeTraits>::run() {
  // Parsing library invokes every chained command handler, but we only need
  // the 'leaf' command handler to be invoked. Thus, after the first (leaf)
  // command handler is invoked, simply return.
  // TODO: explore if the parsing library provides a better way to implement
  // this.
  if (hasRun) {
    return;
  }

  hasRun = true;

  utils::setLogLevel(CmdGlobalOptions::getInstance()->getLogLevel());
  utils::logUsage(folly::demangle(typeid(this)).toStdString());

  auto extraOptionsEC =
      CmdGlobalOptions::getInstance()->validateNonFilterOptions();
  if (extraOptionsEC != CmdGlobalOptions::CliOptionResult::EOK) {
    exit(EINVAL);
  }

  /* If there are errors during filter parsing, we do not exit in the getFilters
  method of CmdGlobalOptions.cpp. Instead, we get the parsing error code and
  exit here. This is to avoid having living references during the
  destrowInstances time!
  */
  auto filterParsingEC = CmdGlobalOptions::CliOptionResult::EOK;
  auto parsedFilters =
      CmdGlobalOptions::getInstance()->getFilters(filterParsingEC);
  if (filterParsingEC != CmdGlobalOptions::CliOptionResult::EOK) {
    exit(EINVAL);
  }

  if (!parsedFilters.empty()) {
    const auto& validFilters = impl().getValidFilters();
    const auto& errorCode =
        CmdGlobalOptions::getInstance()->isValid(validFilters, parsedFilters);
    if (!(errorCode == CmdGlobalOptions::CliOptionResult::EOK)) {
      exit(EINVAL);
    }
  }

  auto hosts = getHosts();

  std::vector<std::shared_future<std::tuple<std::string, RetType, std::string>>>
      futureList;
  for (const auto& host : hosts) {
    futureList.push_back(std::async(
                             std::launch::async,
                             &CmdHandler::asyncHandler,
                             this,
                             host /*, inArgs*/)
                             .share());
  }

  // TODO(surabhi236): perform the actual filtering here (Intern milestone 4).

  if (CmdGlobalOptions::getInstance()->getFmt().isJson()) {
    printJson(impl(), futureList, std::cout, std::cerr);
  } else {
    printTabular(impl(), futureList, std::cout, std::cerr);
  }

  for (auto& fut : futureList) {
    auto [host, data, errStr] = fut.get();
    // exit with failure if any of the calls failed
    if (!errStr.empty()) {
      exit(1);
    }
  }
}

} // namespace facebook::fboss
