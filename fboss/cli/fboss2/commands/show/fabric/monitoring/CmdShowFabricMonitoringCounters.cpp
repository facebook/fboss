// Copyright 2004-present Facebook. All Rights Reserved.

#include "CmdShowFabricMonitoringCounters.h"

#include <folly/Conv.h>
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;
using ObjectArgType = CmdShowFabricMonitoringCountersTraits::ObjectArgType;
using RetType = CmdShowFabricMonitoringCountersTraits::RetType;

RetType CmdShowFabricMonitoringCounters::queryClient(const HostInfo& hostInfo) {
  std::map<int32_t, FabricLinkMonPortStats> stats;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getAllPortInfo(portInfo);
  client->sync_getAllFabricLinkMonitoringStats(stats);
  return createModel(stats, portInfo);
}

void CmdShowFabricMonitoringCounters::printOutput(
    const RetType& model,
    std::ostream& out) {
  Table table;
  table.setHeader(
      {"Port Name",
       "Port ID",
       "TX Count",
       "RX Count",
       "Dropped Count",
       "Invalid Payload Count",
       "No Pending SeqNum Count"});

  for (const auto& entry : model.counters().value()) {
    table.addRow(
        {entry.portName().value(),
         std::to_string(folly::copy(entry.portId().value())),
         std::to_string(folly::copy(entry.txCount().value())),
         std::to_string(folly::copy(entry.rxCount().value())),
         std::to_string(folly::copy(entry.droppedCount().value())),
         std::to_string(folly::copy(entry.invalidPayloadCount().value())),
         std::to_string(folly::copy(entry.noPendingSeqNumCount().value()))});
  }
  out << table << std::endl;
}

RetType CmdShowFabricMonitoringCounters::createModel(
    const std::map<int32_t, FabricLinkMonPortStats>& stats,
    const std::map<int32_t, facebook::fboss::PortInfoThrift>& portInfo) {
  RetType model;

  for (const auto& [portID, portStats] : stats) {
    cli::FabricMonitoringCounter counter;

    counter.portId() = portID;

    // Set port name, defaulting to port ID if not found
    auto portInfoIt = portInfo.find(portID);
    if (portInfoIt != portInfo.end()) {
      counter.portName() = portInfoIt->second.name().value();
    } else {
      counter.portName() = folly::to<std::string>(portID);
    }

    counter.txCount() = folly::copy(portStats.txCount().value());
    counter.rxCount() = folly::copy(portStats.rxCount().value());
    counter.droppedCount() = folly::copy(portStats.droppedCount().value());
    counter.invalidPayloadCount() =
        folly::copy(portStats.invalidPayloadCount().value());
    counter.noPendingSeqNumCount() =
        folly::copy(portStats.noPendingSeqNumCount().value());

    model.counters()->push_back(counter);
  }

  // Sort o/p by port name
  std::sort(
      model.counters()->begin(),
      model.counters()->end(),
      [](const cli::FabricMonitoringCounter& counter1,
         const cli::FabricMonitoringCounter& counter2) {
        return utils::comparePortName(
            counter1.portName().value(), counter2.portName().value());
      });

  return model;
}

} // namespace facebook::fboss
