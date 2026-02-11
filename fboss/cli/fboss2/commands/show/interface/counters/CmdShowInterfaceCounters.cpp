/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"

#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

CmdShowInterfaceCounters::RetType CmdShowInterfaceCounters::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedIfs) {
  RetType countersEntries;

  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters;
  client->sync_getAllPortInfo(portCounters);

  return createModel(portCounters, queriedIfs);
}

CmdShowInterfaceCounters::RetType CmdShowInterfaceCounters::createModel(
    const std::map<int32_t, facebook::fboss::PortInfoThrift>& portCounters,
    const std::vector<std::string>& queriedIfs) {
  RetType ret;

  std::unordered_set<std::string> queriedSet(
      queriedIfs.begin(), queriedIfs.end());

  for (const auto& port : portCounters) {
    auto portInfo = port.second;
    if (queriedIfs.size() == 0 || queriedSet.count(portInfo.name().value())) {
      cli::InterfaceCounters counter;

      counter.interfaceName() = portInfo.name().value();
      counter.inputBytes() =
          folly::copy(portInfo.input().value().bytes().value());
      counter.inputUcastPkts() =
          folly::copy(portInfo.input().value().ucastPkts().value());
      counter.inputMulticastPkts() =
          folly::copy(portInfo.input().value().multicastPkts().value());
      counter.inputBroadcastPkts() =
          folly::copy(portInfo.input().value().broadcastPkts().value());
      counter.outputBytes() =
          folly::copy(portInfo.output().value().bytes().value());
      counter.outputUcastPkts() =
          folly::copy(portInfo.output().value().ucastPkts().value());
      counter.outputMulticastPkts() =
          folly::copy(portInfo.output().value().multicastPkts().value());
      counter.outputBroadcastPkts() =
          folly::copy(portInfo.output().value().broadcastPkts().value());

      ret.int_counters()->push_back(counter);
    }
  }

  std::sort(
      ret.int_counters()->begin(),
      ret.int_counters()->end(),
      [](cli::InterfaceCounters& a, cli::InterfaceCounters b) {
        return a.interfaceName().value() < b.interfaceName().value();
      });

  return ret;
}

void CmdShowInterfaceCounters::printOutput(
    const RetType& model,
    std::ostream& out) {
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
  for (const auto& counter : model.int_counters().value()) {
    table.addRow({
        counter.interfaceName().value(),
        makeStr(folly::copy(counter.inputBytes().value())),
        makeStr(folly::copy(counter.inputUcastPkts().value())),
        makeStr(folly::copy(counter.inputMulticastPkts().value())),
        makeStr(folly::copy(counter.inputBroadcastPkts().value())),
        makeStr(folly::copy(counter.outputBytes().value())),
        makeStr(folly::copy(counter.outputUcastPkts().value())),
        makeStr(folly::copy(counter.outputMulticastPkts().value())),
        makeStr(folly::copy(counter.outputBroadcastPkts().value())),

    });
  }

  out << table << std::endl;
}

} // namespace facebook::fboss
