/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#ifndef IS_OSS
#include "common/thrift/thrift/gen-cpp2/MonitorAsyncClient.h"
#endif
#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/traffic/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/executors/IOThreadPoolExecutor.h"

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <re2/re2.h>
#include <string>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceTrafficTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceTrafficModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceTraffic : public CmdHandler<
                                    CmdShowInterfaceTraffic,
                                    CmdShowInterfaceTrafficTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceTrafficTraits::ObjectArgType;
  using RetType = CmdShowInterfaceTrafficTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    folly::IOThreadPoolExecutor executor(2);

    // Gather port stats asynchrounously
    auto portInfos =
        client->semifuture_getAllPortInfo().via(executor.getEventBase());

    std::map<std::string, int64_t> counters;
    if (utils::isFbossFeatureEnabled(hostInfo.getName(), "multi_switch")) {
#ifndef IS_OSS
      auto hwAgentQueryFn =
          [&counters](
              apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
            std::map<std::string, int64_t> hwAgentCounters;
            apache::thrift::Client<facebook::thrift::Monitor> monitoringClient{
                client.getChannelShared()};
            monitoringClient.sync_getCounters(hwAgentCounters);
            counters.merge(hwAgentCounters);
          };
      utils::runOnAllHwAgents(hostInfo, hwAgentQueryFn);
#endif
    } else {
      auto entries =
          client->semifuture_getCounters().via(executor.getEventBase());
      entries.wait();
      counters = entries.value();
    }

    portInfos.wait();

    return createModel(portInfos.value(), counters, queriedIfs);
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters,
      std::map<std::string, int64_t> intCounters,
      const std::vector<std::string>& queriedIfs) {
    RetType ret;

    std::unordered_set<std::string> queriedSet(
        queriedIfs.begin(), queriedIfs.end());

    // Create counter object for each interface
    for (const auto& portEntry : portCounters) {
      auto portInfo = portEntry.second;
      if (queriedIfs.size() == 0 || queriedSet.count(portInfo.get_name())) {
        cli::TrafficErrorCounters errorCounters;

        std::string pname = portInfo.get_name();

        // FBOSS devices only have 2 error counters, so extract those here
        // The other counters are placeholders to match EOS output
        int64_t inputErrors = portInfo.get_input().get_errors().get_errors();
        int64_t outputErrors = portInfo.get_output().get_errors().get_errors();

        const auto operState = portInfo.get_operState();

        // As mentioned above, only tx and rx errors are meaningful to FBOSS.
        // The other fields are statically set just to provide output parity
        // with EOS
        errorCounters.interfaceName() = pname;
        errorCounters.peerIf() =
            extractExpectedPort(portInfo.get_description());
        errorCounters.ifStatus() =
            (operState == facebook::fboss::PortOperState::UP) ? "up" : "down";
        errorCounters.fcsErrors() = 0;
        errorCounters.alignErrors() = 0;
        errorCounters.symbolErrors() = 0;
        errorCounters.rxErrors() = inputErrors;
        errorCounters.runtErrors() = 0;
        errorCounters.giantErrors() = 0;
        errorCounters.txErrors() = outputErrors;

        cli::TrafficCounters trafficCounters;
        // Getting various counters and converting to Mbps
        int64_t portSpeed = portInfo.get_speedMbps();

        long inSpeedBps = intCounters[pname + ".in_bytes.rate.60"] * 8;

        double inSpeedMbps = (double)inSpeedBps / 1000000;

        // Unicast + multicast + broadcast = PPS
        int64_t inPPS = intCounters[pname + ".in_unicast_pkts.rate.60"] +
            intCounters[pname + ".in_multicast_pkts.rate.60"] +
            intCounters[pname + ".in_broadcast_pkts.rate.60"];

        long outSpeedBps = intCounters[pname + ".out_bytes.rate.60"] * 8;

        double outSpeedMbps = (double)outSpeedBps / 1000000;

        // Unicast + multicast + broadcast = PPS
        int64_t outPPS = intCounters[pname + ".out_unicast_pkts.rate.60"] +
            intCounters[pname + ".out_multicast_pkts.rate.60"] +
            intCounters[pname + ".out_broadcast_pkts.rate.60"];

        trafficCounters.interfaceName() = pname;
        trafficCounters.peerIf() =
            extractExpectedPort(portInfo.get_description());
        trafficCounters.inMbps() = inSpeedMbps;
        trafficCounters.inPct() =
            calculateUtilizationPercent(inSpeedMbps, portSpeed);
        trafficCounters.inKpps() = inPPS / 1000;
        trafficCounters.outMbps() = outSpeedMbps;
        trafficCounters.outPct() =
            calculateUtilizationPercent(outSpeedMbps, portSpeed);
        trafficCounters.outKpps() = outPPS / 1000;
        trafficCounters.portSpeed() = portSpeed;

        // The original in fb_toolkit has an option for "show zero".  Since we
        // can't accept arbitrarily deep parameters right now we will default to
        // show non-zero and revisit as an option down the road
        if (isNonZeroErrors(errorCounters)) {
          ret.error_counters()->push_back(errorCounters);
        }

        if (isInterestingTraffic(trafficCounters)) {
          ret.traffic_counters()->push_back(trafficCounters);
        }
      }
    }
    std::sort(
        ret.error_counters()->begin(),
        ret.error_counters()->end(),
        [](cli::TrafficErrorCounters& a, cli::TrafficErrorCounters b) {
          return a.get_interfaceName() < b.get_interfaceName();
        });

    std::sort(
        ret.traffic_counters()->begin(),
        ret.traffic_counters()->end(),
        [](cli::TrafficCounters& a, cli::TrafficCounters b) {
          return a.get_interfaceName() < b.get_interfaceName();
        });
    return ret;
  }

  double calculateUtilizationPercent(double speedMbps, int bandwidth) {
    return (speedMbps / bandwidth) * 100;
  }

  std::string extractExpectedPort(const std::string& portDescription) {
    /*
    Network interfaces have descriptions that indicated the expected remote
    peer. Unfortunately we seem to have at least 3 different standards for
    interface descriptions depending upon platform.  Examples noted below:

    Minipack/Minipack2:
     u-013: ssw013.s001 (F=spine:L=d-050)
     d-001: rsw001.p050 (F=rack:L=u-001:D=vll2.0001.0050.0001)

    Glacier/Glacier2:
     et6-1-1.fa001-du001.ftw5:T=usIF:U=facebook
     d-001: fsw001.p001 (F=fabric:L=u-001)

    Wedge:
     fsw001.p062.f01.vll3:eth4/1/1

    This function uses the BOOST split function to parse out the hostname
    portion to be used as the expected peer

    Return a string with the expected peer
    */
    if (portDescription.length() == 0) {
      return "--";
    }
    std::vector<std::string> results;
    // First try splitting as if it's a minipack
    boost::split(results, portDescription, [](char c) { return c == ' '; });
    // If we don't get results from the split then split on "." which is a
    // different format
    if (results.size() <= 1) {
      boost::split(results, portDescription, [](char c) { return c == '.'; });

      // Finally, if it's an RSW we need to handle that differently
      const RE2 ssw_regex(".*ssw.*");
      if (RE2::FullMatch(portDescription, ssw_regex)) {
        return results[1] + "." + results[2];
      }
      const RE2 fsw_regex("^fsw.*");
      if (RE2::FullMatch(portDescription, fsw_regex)) {
        return results[0] + "." + results[1];
      }
    }
    return results[1];
  }

  bool isInterestingTraffic(cli::TrafficCounters& tc) {
    if (tc.get_inMbps() < 0.1 && tc.get_inPct() < 0.1 &&
        tc.get_inKpps() < 0.1 && tc.get_outMbps() < 0.1 &&
        tc.get_outPct() < 0.1 && tc.get_outKpps() < 0.1) {
      return false;
    }
    return true;
  }

  bool isNonZeroErrors(cli::TrafficErrorCounters& ec) {
    if (ec.get_rxErrors() <= 0 && ec.get_txErrors() <= 0) {
      return false;
    }
    return true;
  }

  std::vector<double> getTrafficTotals(
      std::vector<cli::TrafficCounters> trafficCounters) {
    double inMbpsT = 0.0;
    double inPctT = 0.0;
    double inKppsT = 0.0;
    double outMbpsT = 0.0;
    double outPctT = 0.0;
    double outKppsT = 0.0;
    double totalBW = 0.0;

    for (auto& tc : trafficCounters) {
      inMbpsT += tc.get_inMbps();
      inKppsT += tc.get_inKpps();
      outMbpsT += tc.get_outMbps();
      outKppsT += tc.get_outKpps();
      totalBW += tc.get_portSpeed();
    }

    inPctT = (inMbpsT / totalBW) * 100;
    outPctT = (outMbpsT / totalBW) * 100;

    std::vector<double> rv{
        inMbpsT, inPctT, inKppsT, outMbpsT, outPctT, outKppsT};
    return rv;
  }

  std::string getThresholdColor(double pct) {
    if (pct >= 70) {
      return "ERROR";
    } else if (pct >= 50) {
      return "WARN";
    } else if (pct > 0) {
      return "GOOD";
    }
    return "NONE";
  }

  std::vector<std::string> getRowColors(double inPct, double outPct) {
    // First get color based on percent threshold
    std::string inColor = getThresholdColor(inPct);
    std::string outColor = getThresholdColor(outPct);

    // Now decide what to color the interface name
    std::string ifNameColor;
    if (inPct > outPct) {
      ifNameColor = inColor;
    } else {
      ifNameColor = outColor;
    }

    // Colors as a warning if we seem to have traffic in a single direction
    const int UNIDRECTIONAL_TRAFFIC_THRESHOLD = 1;

    if ((inPct >= UNIDRECTIONAL_TRAFFIC_THRESHOLD &&
         outPct < UNIDRECTIONAL_TRAFFIC_THRESHOLD) ||
        (inPct < UNIDRECTIONAL_TRAFFIC_THRESHOLD &&
         outPct >= UNIDRECTIONAL_TRAFFIC_THRESHOLD)) {
      if (ifNameColor != "ERROR") {
        ifNameColor = "WARN";
      }
    }

    std::vector<std::string> rowColors = {inColor, outColor, ifNameColor};
    return rowColors;
  }

  Table::StyledCell makeColorCell(std::string val, std::string level) {
    if (level == "ERROR") {
      return Table::StyledCell(val, Table::Style::ERROR);
    } else if (level == "GOOD") {
      return Table::StyledCell(val, Table::Style::GOOD);
    } else if (level == "INFO") {
      return Table::StyledCell(val, Table::Style::INFO);
    } else if (level == "WARN") {
      return Table::StyledCell(val, Table::Style::WARN);
    }
    return Table::StyledCell(val, Table::Style::NONE);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table errorTable;
    Table trafficTable;

    // If Global option for color is yes (default) then print color.  Otherwise
    // suppress color
    bool printColor = false;
    if ((CmdGlobalOptions::getInstance()->getColor() == "yes") &&
        isatty(fileno(stdout))) {
      printColor = true;
    }

    if (model.get_error_counters().size() != 0) {
      constexpr std::string_view errorsString =
          "ERRORS {} interfaces, watch for any incrementing counters:\n";

      if (printColor) {
        fmt::print(
            fg(fmt::color::red),
            errorsString,
            std::to_string(model.get_error_counters().size()));
      } else {
        out << fmt::format(
            errorsString, std::to_string(model.get_error_counters().size()));
      }

      errorTable.setHeader({
          "Interface Name",
          "Peer",
          "Status",
          "FCS",
          "Align",
          "Symbol",
          "Rx",
          "Runts",
          "Giants",
          "Tx",
      });

      for (const auto& counter : model.get_error_counters()) {
        errorTable.addRow({
            makeColorCell(counter.get_interfaceName(), "ERROR"),
            makeColorCell(counter.get_peerIf(), "ERROR"),
            makeColorCell(counter.get_ifStatus(), "ERROR"),
            makeColorCell(std::to_string(counter.get_fcsErrors()), "ERROR"),
            makeColorCell(std::to_string(counter.get_alignErrors()), "ERROR"),
            makeColorCell(std::to_string(counter.get_symbolErrors()), "ERROR"),
            makeColorCell(std::to_string(counter.get_rxErrors()), "ERROR"),
            makeColorCell(std::to_string(counter.get_runtErrors()), "ERROR"),
            makeColorCell(std::to_string(counter.get_giantErrors()), "ERROR"),
            makeColorCell(std::to_string(counter.get_txErrors()), "ERROR"),
        });
      }

    } else {
      constexpr std::string_view noErrorString =
          "No interfaces with In/Out errors - all-clear!\n";
      if (printColor) {
        fmt::print(fg(fmt::color::green), noErrorString);
      } else {
        out << noErrorString;
      }
    }

    trafficTable.setHeader({
        "Interface Name",
        "Peer",
        "Intvl",
        "InMbps",
        "InPct",
        "InKpps",
        "OutMbps",
        "OutPct",
        "OutKpps",
    });

    for (const auto& trafficCounter : model.get_traffic_counters()) {
      std::vector<std::string> rowColors =
          getRowColors(trafficCounter.get_inPct(), trafficCounter.get_outPct());

      std::string ifNameColor = rowColors[2];
      std::string rxColor = rowColors[0];
      std::string txColor = rowColors[1];

      trafficTable.addRow(
          {makeColorCell(trafficCounter.get_interfaceName(), ifNameColor),
           makeColorCell(trafficCounter.get_peerIf(), ifNameColor),
           makeColorCell("0:60", ifNameColor),
           makeColorCell(
               fmt::format("{:.2f}", trafficCounter.get_inMbps()), rxColor),
           makeColorCell(
               fmt::format("{:.2f}", trafficCounter.get_inPct()) + "%",
               rxColor),
           makeColorCell(
               fmt::format("{:.2f}", trafficCounter.get_inKpps()), rxColor),
           makeColorCell(
               fmt::format("{:.2f}", trafficCounter.get_outMbps()), txColor),
           makeColorCell(
               fmt::format("{:.2f}", trafficCounter.get_outPct()) + "%",
               txColor),
           makeColorCell(
               fmt::format("{:.2f}", trafficCounter.get_outKpps()), txColor)});
    }

    std::vector<double> totalTraffic =
        getTrafficTotals(model.get_traffic_counters());

    trafficTable.addRow({
        makeColorCell("Total", "INFO"),
        makeColorCell("--", "INFO"),
        makeColorCell("--", "INFO"),
        makeColorCell(fmt::format("{:.2f}", totalTraffic[0]), "INFO"),
        makeColorCell(fmt::format("{:.2f}", totalTraffic[1]) + "%", "INFO"),
        makeColorCell(fmt::format("{:.2f}", totalTraffic[2]), "INFO"),
        makeColorCell(fmt::format("{:.2f}", totalTraffic[3]), "INFO"),
        makeColorCell(fmt::format("{:.2f}", totalTraffic[4]) + "%", "INFO"),
        makeColorCell(fmt::format("{:.2f}", totalTraffic[5]), "INFO"),
    });

    out << errorTable << std::endl;
    out << trafficTable << std::endl;
  }
};

} // namespace facebook::fboss
