/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowCpuPort.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowCpuPortTraits::RetType;

RetType CmdShowCpuPort::queryClient(const HostInfo& hostInfo) {
  std::map<int32_t, facebook::fboss::CpuPortStats> cpuPortStatEntries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getAllCpuPortStats(cpuPortStatEntries);
  return createModel(cpuPortStatEntries);
}

void CmdShowCpuPort::printOutput(const RetType& model, std::ostream& out) {
  std::vector<std::string> detailedOutput;

  Table table;
  table.setHeader(
      {"Switch ID",
       "CPU Queue ID",
       "Queue Name",
       "Ingress Packets",
       "Discard Packets"});
  for (const auto& cpuPortEntry : model.cpuPortStatEntries().value()) {
    for (const auto& cpuQueueEntry : cpuPortEntry.second) {
      table.addRow(
          {folly::to<std::string>(folly::copy(cpuPortEntry.first)),
           folly::to<std::string>(folly::copy(cpuQueueEntry.id().value())),
           cpuQueueEntry.name().value(),
           folly::to<std::string>(
               folly::copy(cpuQueueEntry.ingressPackets().value())),
           folly::to<std::string>(
               folly::copy(cpuQueueEntry.discardPackets().value()))});
    }
  }
  out << table << std::endl;
}

RetType CmdShowCpuPort::createModel(
    std::map<int32_t, facebook::fboss::CpuPortStats>& cpuPortStatEntries) {
  RetType model;

  for (auto& [switchId, cpuPortStats] : cpuPortStatEntries) {
    for (const auto& queueId2Name : cpuPortStats.queueToName_().value()) {
      cli::CpuPortQueueEntry cpuPortQueueEntry;
      cpuPortQueueEntry.id() = queueId2Name.first;
      cpuPortQueueEntry.name() = queueId2Name.second;
      const auto& ingressPktMap = cpuPortStats.queueInPackets_().value();
      const auto& ingressPktIter = ingressPktMap.find(queueId2Name.first);
      if (ingressPktIter != cpuPortStats.queueInPackets_().value().end()) {
        cpuPortQueueEntry.ingressPackets() = ingressPktIter->second;
      }
      const auto& discardPktMap = cpuPortStats.queueDiscardPackets_().value();
      const auto& discardPktIter = discardPktMap.find(queueId2Name.first);
      if (discardPktIter != cpuPortStats.queueDiscardPackets_().value().end()) {
        cpuPortQueueEntry.discardPackets() = discardPktIter->second;
      }
      model.cpuPortStatEntries()[switchId].push_back(cpuPortQueueEntry);
    }
  }
  return model;
}

std::string_view CmdShowCpuPortTraits::description() {
  return "Displays per-CPU-queue statistics for each switch: the queue ID and name, ingress packet count, and discarded packet count. Use it to see how CPU-bound traffic is classified and whether any CPU queue is dropping.";
}

RetType CmdShowCpuPort::sampleModel() {
  RetType model;

  cli::CpuPortQueueEntry entry0, entry1, entry2, entry3;

  entry0.id() = 0;
  entry0.name() = "cpuQueue-low";
  entry0.ingressPackets() = 532819;
  entry0.discardPackets() = 431625;

  entry1.id() = 1;
  entry1.name() = "cpuQueue-default";
  entry1.ingressPackets() = 0;
  entry1.discardPackets() = 0;

  entry2.id() = 2;
  entry2.name() = "cpuQueue-mid";
  entry2.ingressPackets() = 93986419;
  entry2.discardPackets() = 0;

  entry3.id() = 9;
  entry3.name() = "cpuQueue-high";
  entry3.ingressPackets() = 182542535;
  entry3.discardPackets() = 0;

  model.cpuPortStatEntries()[0] = {entry0, entry1, entry2, entry3};

  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowCpuPort, CmdShowCpuPortTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowCpuPort, CmdShowCpuPortTraits>::getValidFilters();

} // namespace facebook::fboss
