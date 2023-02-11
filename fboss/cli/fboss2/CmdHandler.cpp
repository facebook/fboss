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
#include <folly/lang/Exception.h>
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
#include "fboss/cli/fboss2/commands/show/fabric/CmdShowFabric.h"
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
#include "fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h"
#include "fboss/cli/fboss2/commands/show/mac/CmdShowMacAddrToBlock.h"
#include "fboss/cli/fboss2/commands/show/mac/CmdShowMacDetails.h"
#include "fboss/cli/fboss2/commands/show/mpls/CmdShowMplsRoute.h"
#include "fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPortQueue.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteDetails.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteSummary.h"
#include "fboss/cli/fboss2/commands/show/sdk/dump/CmdShowSdkDump.h"
#include "fboss/cli/fboss2/commands/show/teflow/CmdShowTeFlow.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "folly/futures/Future.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include <cstring>
#include <future>
#include <iostream>
#include <stdexcept>

#include "fboss/cli/fboss2/commands/show/acl/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/arp/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/fabric/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/phy/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/status/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/interface/traffic/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/lldp/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/mac/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/ndp/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/commands/show/transceiver/gen-cpp2/model_visitation.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/fboss_types.h"
#include "fboss/agent/if/gen-cpp2/fboss_visitation.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_visitation.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_visitation.h"
#include "fboss/lib/phy/gen-cpp2/prbs_visitation.h"

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

template <typename CmdTypeT>
void printAggregate(
    const std::optional<facebook::fboss::CmdGlobalOptions::AggregateOption>&
        parsedAgg,
    std::vector<std::shared_future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    const facebook::fboss::ValidAggMapType& validAggMap) {
  if (!parsedAgg->acrossHosts) {
    for (auto& result : results) {
      auto [host, data, errStr] = result.get();
      if (errStr.empty()) {
        std::cout << host << " Aggregation result:: "
                  << facebook::fboss::performAggregation<CmdTypeT>(
                         data, parsedAgg, validAggMap)
                  << std::endl;
      } else {
        std::cerr << host << "::" << std::endl
                  << std::string(80, '=') << std::endl;
        std::cerr << errStr << std::endl << std::endl;
      }
    }
  } else {
    // double because aggregation results are doubles.
    std::vector<double> hostAggResults;
    for (auto& result : results) {
      auto [host, data, errStr] = result.get();
      if (errStr.empty()) {
        hostAggResults.push_back(facebook::fboss::performAggregation<CmdTypeT>(
            data, parsedAgg, validAggMap));
      } else {
        std::cerr << host << "::" << std::endl
                  << std::string(80, '=') << std::endl;
        std::cerr << errStr << std::endl << std::endl;
      }
    }

    double aggregateAcrossHosts;
    int rowNumber = 0;
    const auto& aggOp = parsedAgg->aggOp;
    const auto& aggColumn = parsedAgg->columnName;
    auto it = validAggMap.find(aggColumn);
    for (auto result : hostAggResults) {
      if (rowNumber == 0) {
        if (aggOp != facebook::fboss::AggregateOpEnum::COUNT) {
          aggregateAcrossHosts =
              (it->second)->getInitValue(std::to_string(result), aggOp);
        } else {
          // can't use init value from the countAgg() here, hence separate
          // handling
          aggregateAcrossHosts = result;
        }
        rowNumber += 1;
      } else {
        switch (aggOp) {
          case facebook::fboss::AggregateOpEnum::SUM:
            aggregateAcrossHosts = facebook::fboss::SumAgg<double>().accumulate(
                result, aggregateAcrossHosts);

            break;
          case facebook::fboss::AggregateOpEnum::MIN:
            aggregateAcrossHosts = facebook::fboss::MinAgg<double>().accumulate(
                result, aggregateAcrossHosts);
            break;
          case facebook::fboss::AggregateOpEnum::MAX:
            aggregateAcrossHosts = facebook::fboss::MaxAgg<double>().accumulate(
                result, aggregateAcrossHosts);
            break;
          // when aggregating counts across hosts, we need to sum them!
          case facebook::fboss::AggregateOpEnum::COUNT:
            aggregateAcrossHosts = facebook::fboss::SumAgg<double>().accumulate(
                result, aggregateAcrossHosts);
            break;
          case facebook::fboss::AggregateOpEnum::AVG:
            std::cerr << "Average aggregation not supported yet!" << std::endl;
            break;
        }
      }
    }
    std::cout << "Result of aggregating across all hosts: "
              << aggregateAcrossHosts << std::endl;
  }
}

namespace facebook::fboss {

// Avoid template linker error
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void CmdHandler<CmdShowAcl, CmdShowAclTraits>::run();
template void CmdHandler<CmdShowAgentSsl, CmdShowAgentSslTraits>::run();
template void
CmdHandler<CmdShowAggregatePort, CmdShowAggregatePortTraits>::run();
template void CmdHandler<CmdShowArp, CmdShowArpTraits>::run();
template void CmdHandler<CmdShowFabric, CmdShowFabricTraits>::run();
template void CmdHandler<CmdShowLldp, CmdShowLldpTraits>::run();
template void
CmdHandler<CmdShowMacAddrToBlock, CmdShowMacAddrToBlockTraits>::run();
template void CmdHandler<CmdShowMacDetails, CmdShowMacDetailsTraits>::run();
template void CmdHandler<CmdShowNdp, CmdShowNdpTraits>::run();
template void CmdHandler<CmdShowPort, CmdShowPortTraits>::run();
template void CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits>::run();
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

template const ValidFilterMapType
CmdHandler<CmdShowArp, CmdShowArpTraits>::getValidFilters();
template const ValidFilterMapType
CmdHandler<CmdShowFabric, CmdShowFabricTraits>::getValidFilters();
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

static bool hasRun = false;

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdHandler<CmdTypeT, CmdTypeTraits>::runHelper() {
  // Parsing library invokes every chained command handler, but we only need
  // the 'leaf' command handler to be invoked. Thus, after the first (leaf)
  // command handler is invoked, simply return.
  // TODO: explore if the parsing library provides a better way to implement
  // this.
  if (hasRun) {
    return;
  }

  hasRun = true;
  auto extraOptionsEC =
      CmdGlobalOptions::getInstance()->validateNonFilterOptions();
  if (extraOptionsEC != cli::CliOptionResult::EOK) {
    throw std::invalid_argument(folly::to<std::string>(
        "Error in filter parsing: ",
        apache::thrift::util::enumNameSafe(extraOptionsEC)));
  }

  /* If there are errors during filter parsing, we do not exit in the getFilters
  method of CmdGlobalOptions.cpp. Instead, we get the parsing error code and
  exit here. This is to avoid having living references during the
  destrowInstances time!
  */
  auto filterParsingEC = cli::CliOptionResult::EOK;
  auto parsedFilters =
      CmdGlobalOptions::getInstance()->getFilters(filterParsingEC);
  if (filterParsingEC != cli::CliOptionResult::EOK) {
    throw std::invalid_argument(folly::to<std::string>(
        "Error in filter parsing: ",
        apache::thrift::util::enumNameSafe(filterParsingEC)));
  }

  ValidFilterMapType validFilters = {};
  if (!parsedFilters.empty()) {
    validFilters = getValidFilters();
    const auto& errorCode =
        CmdGlobalOptions::getInstance()->isValid(validFilters, parsedFilters);
    if (!(errorCode == cli::CliOptionResult::EOK)) {
      throw std::invalid_argument(folly::to<std::string>(
          "Error in filter parsing: ",
          apache::thrift::util::enumNameSafe(errorCode)));
    }
  }

  auto aggParsingEC = cli::CliOptionResult::EOK;
  auto parsedAggregationInput =
      CmdGlobalOptions::getInstance()->parseAggregate(aggParsingEC);

  if (aggParsingEC != cli::CliOptionResult::EOK) {
    throw std::invalid_argument(folly::to<std::string>(
        "Error in aggregate parsing: ",
        apache::thrift::util::enumNameSafe(aggParsingEC)));
  }

  ValidAggMapType validAggs = {};
  if (parsedAggregationInput.has_value()) {
    validAggs = getValidAggs();
    const auto& errorCode = CmdGlobalOptions::getInstance()->isValidAggregate(
        validAggs, parsedAggregationInput);
    if (errorCode != cli::CliOptionResult::EOK) {
      throw std::invalid_argument(folly::to<std::string>(
          "Error in Aggregate validation: ",
          apache::thrift::util::enumNameSafe(errorCode)));
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
                             host,
                             parsedFilters,
                             validFilters)
                             .share());
  }

  if (!parsedAggregationInput.has_value()) {
    if (CmdGlobalOptions::getInstance()->getFmt().isJson()) {
      printJson(impl(), futureList, std::cout, std::cerr);
    } else {
      printTabular(impl(), futureList, std::cout, std::cerr);
    }
  } else {
    printAggregate<CmdTypeT>(parsedAggregationInput, futureList, validAggs);
  }

  for (auto& fut : futureList) {
    auto [host, data, errStr] = fut.get();
    // exit with failure if any of the calls failed
    if (!errStr.empty()) {
      throw std::runtime_error("Error in command execution");
    }
  }
}

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdHandler<CmdTypeT, CmdTypeTraits>::run() {
  utils::setLogLevel(CmdGlobalOptions::getInstance()->getLogLevel());

  // setup a stop_watch for total time to run command
  folly::stop_watch<> watch;
  utils::CmdLogInfo cmdLogInfo;

  try {
    // The MACROS are executed in reversed order
    // In order to have SCOPE_EXIT be the last one to get executed,
    // it must come before SCOPE_FAIL and SCOPE_SUCCESS
    SCOPE_EXIT {
      cmdLogInfo.CmdName = folly::demangle(typeid(this)).toStdString();
      cmdLogInfo.Arguments = CmdArgsLists::getInstance()->getArgStr();
      cmdLogInfo.Duration = utils::getDurationStr(watch);
      cmdLogInfo.UserInfo = utils::getUserInfo();
      utils::logUsage(cmdLogInfo);
    };

    SCOPE_FAIL {
      cmdLogInfo.ExitStatus = "FAILURE";
    };

    SCOPE_SUCCESS {
      cmdLogInfo.ExitStatus = "SUCCESS";
    };

    runHelper();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }
}

/* Logic: We consider a thrift struct to be filterable only if it is of type
  list and has a single element in it that is another thrift struct.
  The idea is that the outer thrift struct (which corresponds to the RetType()
  in the cmd traits) should contain a list of the actual displayable thrift
  entry. For instance, the ShowPortModel struct has only one elelent that is a
  list of PortEntry struct. This makes it filterable.
  */
template <typename CmdTypeT, typename CmdTypeTraits>
bool CmdHandler<CmdTypeT, CmdTypeTraits>::isFilterable() {
  int numFields = 0;
  bool filterable = true;
  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_FILTERING == true) {
    apache::thrift::for_each_field(
        RetType(), [&](const ThriftField& meta, auto&& /*field_ref*/) {
          const auto& fieldType = *meta.type();
          const auto& thriftBaseType = fieldType.getType();

          if (thriftBaseType != ThriftType::Type::t_list) {
            filterable = false;
          }
          numFields += 1;
        });
  }

  if (numFields != 1) {
    filterable = false;
  }
  return filterable;
}

/* Logic (similar to isFilterable): We consider a thrift struct to be
  amenable to aggregation only if it is of type list and has a single element
  in it that is another thrift struct.
  The idea is that the outer thrift struct (which corresponds to the RetType()
  in the cmd traits) should contain a list of the actual displayable thrift
  entry. For instance, the ShowPortModel struct has only one elelent that is a
  list of PortEntry struct. This makes it aggregatable.
  */
template <typename CmdTypeT, typename CmdTypeTraits>
bool CmdHandler<CmdTypeT, CmdTypeTraits>::isAggregatable() {
  int numFields = 0;
  bool aggregatable = true;
  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_AGGREGATION) {
    apache::thrift::for_each_field(
        RetType(), [&](const ThriftField& meta, auto&& /*field_ref*/) {
          const auto& fieldType = *meta.type();
          const auto& thriftBaseType = fieldType.getType();

          if (thriftBaseType != ThriftType::Type::t_list) {
            aggregatable = false;
          }
          numFields += 1;
        });
  }

  if (numFields != 1) {
    aggregatable = false;
  }
  return aggregatable;
}

template <class T>
using get_value_type_t = typename T::value_type;

/* Logic: We first check if this thrift struct is filterable at all. If it is,
 we constuct a filter map that contains filterable fields. For now, only
 primitive fields will be added to the filter map and considered filterable.
 Also, here we are traversing the nested thrift struct using the visitation api.
 for that, we start with the RetType() object and get to the fields of the inner
 struct using reflection. This is a much cleaner approach than the one that
 redirects through each command's handler to get the inner struct.
 */
template <typename CmdTypeT, typename CmdTypeTraits>
const ValidFilterMapType
CmdHandler<CmdTypeT, CmdTypeTraits>::getValidFilters() {
  if (!CmdHandler<CmdTypeT, CmdTypeTraits>::isFilterable()) {
    return {};
  }
  ValidFilterMapType filterMap;

  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_FILTERING == true) {
    apache::thrift::for_each_field(
        RetType(),
        [&filterMap, this](
            const ThriftField& /*outer_meta*/, auto&& outer_field_ref) {
          if constexpr (apache::thrift::is_thrift_struct_v<folly::detected_or_t<
                            void,
                            get_value_type_t,
                            folly::remove_cvref_t<
                                decltype(*outer_field_ref)>>>) {
            using NestedType = get_value_type_t<
                folly::remove_cvref_t<decltype(*outer_field_ref)>>;
            if constexpr (apache::thrift::is_thrift_struct_v<NestedType>) {
              apache::thrift::for_each_field(
                  NestedType(),
                  [&, this](const ThriftField& meta, auto&& /*field_ref*/) {
                    const auto& fieldType = *meta.type();
                    const auto& thriftBaseType = fieldType.getType();

                    // we are only supporting filtering on primitive typed keys
                    // for now. This will be expanded as we proceed.
                    if (thriftBaseType == ThriftType::Type::t_primitive) {
                      const auto& thriftType = fieldType.get_t_primitive();
                      switch (thriftType) {
                        case ThriftPrimitiveType::THRIFT_STRING_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<std::string>>(
                              *meta.name(),
                              impl().getAcceptedFilterValues()[*meta.name()]);
                          break;

                        case ThriftPrimitiveType::THRIFT_I16_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<int16_t>>(
                              *meta.name());
                          break;

                        case ThriftPrimitiveType::THRIFT_I32_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<int32_t>>(
                              *meta.name());
                          break;

                        case ThriftPrimitiveType::THRIFT_I64_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<int64_t>>(
                              *meta.name());
                          break;

                        case ThriftPrimitiveType::THRIFT_BYTE_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<std::byte>>(
                              *meta.name());
                          break;

                        default:;
                      }
                    }
                  });
            }
          }
        });
  }
  return filterMap;
}

/* Logic: We first check if this thrift struct is aggregatable at all. If it is,
 we constuct an aggregate map that contains aggregatable fields. Only numeric
 and string fields are aggregatable for now. The map maps to the AggInfo object
 that contains information about the operations that are permitted for this
 column. This information is subsequenlty used to perform aggregate validation.
 Also, here we are traversing the nested thrift struct using the visitation api.
 for that, we start with the RetType() object and get to the fields of the inner
 struct using reflection.
 */
template <typename CmdTypeT, typename CmdTypeTraits>
const ValidAggMapType CmdHandler<CmdTypeT, CmdTypeTraits>::getValidAggs() {
  if (!CmdHandler<CmdTypeT, CmdTypeTraits>::isAggregatable()) {
    return {};
  }
  ValidAggMapType aggMap;

  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_AGGREGATION) {
    apache::thrift::for_each_field(
        RetType(),
        [&aggMap, this](
            const ThriftField& /*outer_meta*/, auto&& outer_field_ref) {
          if constexpr (apache::thrift::is_thrift_struct_v<folly::detected_or_t<
                            void,
                            get_value_type_t,
                            folly::remove_cvref_t<
                                decltype(*outer_field_ref)>>>) {
            using NestedType = get_value_type_t<
                folly::remove_cvref_t<decltype(*outer_field_ref)>>;
            if constexpr (apache::thrift::is_thrift_struct_v<NestedType>) {
              apache::thrift::for_each_field(
                  NestedType(),
                  [&, this](const ThriftField& meta, auto&& /*field_ref*/) {
                    const auto& fieldType = *meta.type();
                    const auto& thriftBaseType = fieldType.getType();

                    // Aggregation only supported on numerical keys
                    if (thriftBaseType == ThriftType::Type::t_primitive) {
                      const auto& thriftType = fieldType.get_t_primitive();
                      switch (thriftType) {
                        case ThriftPrimitiveType::THRIFT_I16_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<int16_t>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_I32_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<int32_t>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_I64_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<int64_t>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_FLOAT_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<float>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_DOUBLE_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<double>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_STRING_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<std::string>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::COUNT});

                        default:;
                      }
                    }
                  });
            }
          }
        });
  }
  return aggMap;
}

} // namespace facebook::fboss
