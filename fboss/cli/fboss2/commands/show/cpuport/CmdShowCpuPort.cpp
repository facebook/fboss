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

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowCpuPortTraits::RetType;

RetType CmdShowCpuPort::queryClient(const HostInfo& hostInfo) {
  facebook::fboss::CpuPortStats cpuPortStats;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getCpuPortStats(cpuPortStats);
  return createModel(cpuPortStats);
}

void CmdShowCpuPort::printOutput(const RetType& model, std::ostream& out) {
  std::vector<std::string> detailedOutput;

  Table table;
  table.setHeader(
      {"CPU Queue ID", "Queue Name", "Ingress Packets", "Discard Packets"});

  for (auto const& cpuQueueEntry : model.cpuQueueEntries().value()) {
    table.addRow(
        {folly::to<std::string>(folly::copy(cpuQueueEntry.id().value())),
         cpuQueueEntry.name().value(),
         folly::to<std::string>(
             folly::copy(cpuQueueEntry.ingressPackets().value())),
         folly::to<std::string>(
             folly::copy(cpuQueueEntry.discardPackets().value()))});
  }
  out << table << std::endl;
}

RetType CmdShowCpuPort::createModel(
    facebook::fboss::CpuPortStats& cpuPortStats) {
  RetType model;

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
    model.cpuQueueEntries()->push_back(cpuPortQueueEntry);
  }
  return model;
}

} // namespace facebook::fboss
