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

#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCountersTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceCountersModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceCounters : public CmdHandler<
                                     CmdShowInterfaceCounters,
                                     CmdShowInterfaceCountersTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    RetType countersEntries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters;
    client->sync_getAllPortInfo(portCounters);

    return createModel(portCounters, queriedIfs);
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters,
      const std::vector<std::string>& queriedIfs) {
    RetType ret;

    std::unordered_set<std::string> queriedSet(
        queriedIfs.begin(), queriedIfs.end());

    for (const auto& port : portCounters) {
      auto portInfo = port.second;
      if (queriedIfs.size() == 0 || queriedSet.count(portInfo.get_name())) {
        cli::InterfaceCounters counter;

        counter.interfaceName() = portInfo.get_name();
        counter.inputBytes() = portInfo.get_input().get_bytes();
        counter.inputUcastPkts() = portInfo.get_input().get_ucastPkts();
        counter.inputMulticastPkts() = portInfo.get_input().get_multicastPkts();
        counter.inputBroadcastPkts() = portInfo.get_input().get_broadcastPkts();
        counter.outputBytes() = portInfo.get_output().get_bytes();
        counter.outputUcastPkts() = portInfo.get_output().get_ucastPkts();
        counter.outputMulticastPkts() =
            portInfo.get_output().get_multicastPkts();
        counter.outputBroadcastPkts() =
            portInfo.get_output().get_broadcastPkts();

        ret.int_counters()->push_back(counter);
      }
    }

    std::sort(
        ret.int_counters()->begin(),
        ret.int_counters()->end(),
        [](cli::InterfaceCounters& a, cli::InterfaceCounters b) {
          return a.get_interfaceName() < b.get_interfaceName();
        });

    return ret;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"Interface Name",
         "Bytes(in)",
         "Unicast Pkts(in)",
         "Multicast Pkts(in)",
         "Broadcast Pkts(in)",
         "Bytes(out)",
         "Unicast Pkts(out)",
         "Multicast Pkts(out)",
         "Broadcast Pkts(out)"});

    auto makeStr = [](auto counterVal) -> std::string {
      const std::string kNA = "n/a";
      return counterVal == hardware_stats_constants::STAT_UNINITIALIZED()
          ? kNA
          : std::to_string(counterVal);
    };
    for (const auto& counter : model.get_int_counters()) {
      table.addRow({
          counter.get_interfaceName(),
          makeStr(counter.get_inputBytes()),
          makeStr(counter.get_inputUcastPkts()),
          makeStr(counter.get_inputMulticastPkts()),
          makeStr(counter.get_inputBroadcastPkts()),
          makeStr(counter.get_outputBytes()),
          makeStr(counter.get_outputUcastPkts()),
          makeStr(counter.get_outputMulticastPkts()),
          makeStr(counter.get_outputBroadcastPkts()),

      });
    }

    out << table << std::endl;
  }
};

} // namespace facebook::fboss
