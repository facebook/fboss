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

#include "fboss/cli/fboss2/CmdHandler.h"

#include <thrift/lib/cpp/transport/TTransportException.h>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/cpuport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <unistd.h>
#include <algorithm>

namespace facebook::fboss {

using utils::Table;

struct CmdShowCpuPortTraits : public BaseCommandTraits {
  using RetType = cli::ShowCpuPortModel;
};

class CmdShowCpuPort : public CmdHandler<CmdShowCpuPort, CmdShowCpuPortTraits> {
 public:
  using RetType = CmdShowCpuPortTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    facebook::fboss::CpuPortStats cpuPortStats;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getCpuPortStats(cpuPortStats);
    return createModel(cpuPortStats);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    std::vector<std::string> detailedOutput;

    Table table;
    table.setHeader(
        {"CPU Queue ID", "Queue Name", "Ingress Packets", "Discard Packets"});

    for (auto const& cpuQueueEntry : model.get_cpuQueueEntries()) {
      table.addRow(
          {folly::to<std::string>(cpuQueueEntry.get_id()),
           cpuQueueEntry.get_name(),
           folly::to<std::string>(cpuQueueEntry.get_ingressPackets()),
           folly::to<std::string>(cpuQueueEntry.get_discardPackets())});
    }
    out << table << std::endl;
  }

  RetType createModel(facebook::fboss::CpuPortStats& cpuPortStats) {
    RetType model;

    for (const auto& queueId2Name : cpuPortStats.get_queueToName_()) {
      cli::CpuPortQueueEntry cpuPortQueueEntry;

      cpuPortQueueEntry.id() = queueId2Name.first;
      cpuPortQueueEntry.name() = queueId2Name.second;
      const auto& ingressPktMap = cpuPortStats.get_queueInPackets_();
      const auto& ingressPktIter = ingressPktMap.find(queueId2Name.first);
      if (ingressPktIter != cpuPortStats.get_queueInPackets_().end()) {
        cpuPortQueueEntry.ingressPackets() = ingressPktIter->second;
      }
      const auto& discardPktMap = cpuPortStats.get_queueDiscardPackets_();
      const auto& discardPktIter = discardPktMap.find(queueId2Name.first);
      if (discardPktIter != cpuPortStats.get_queueDiscardPackets_().end()) {
        cpuPortQueueEntry.discardPackets() = discardPktIter->second;
      }
      model.cpuQueueEntries()->push_back(cpuPortQueueEntry);
    }
    return model;
  }
};

} // namespace facebook::fboss
