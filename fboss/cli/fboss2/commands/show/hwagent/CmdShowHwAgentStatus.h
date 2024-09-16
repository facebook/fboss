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
#include "fboss/cli/fboss2/commands/show/hwagent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <unistd.h>
#include <algorithm>

namespace facebook::fboss {

using utils::Table;

struct CmdShowHwAgentStatusTraits : public BaseCommandTraits {
  using RetType = cli::ShowHwAgentStatusModel;
};

class CmdShowHwAgentStatus
    : public CmdHandler<CmdShowHwAgentStatus, CmdShowHwAgentStatusTraits> {
 public:
  using RetType = CmdShowHwAgentStatusTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus> hwAgentStatus;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getHwAgentConnectionStatus(hwAgentStatus);
    MultiSwitchRunState runState;
    client->sync_getMultiSwitchRunState(runState);
    return createModel(hwAgentStatus, runState);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    std::vector<std::string> detailedOutput;

    Table table;
    table.setHeader(
        {"SwitchIndex",
         "SwitchId",
         "State",
         "Link Sync",
         "Stats Sync",
         "Fdb Sync",
         "RxPkt Sync",
         "TxPkt Sync",
         "SwitchReachability Sync"});

    for (auto const& statusEntry : model.get_hwAgentStatusEntries()) {
      table.addRow(
          {folly::to<std::string>(statusEntry.get_switchIndex()),
           folly::to<std::string>(statusEntry.get_switchId()),
           statusEntry.get_runState(),
           folly::to<std::string>(statusEntry.get_linkSyncActive()),
           folly::to<std::string>(statusEntry.get_statsSyncActive()),
           folly::to<std::string>(statusEntry.get_fdbSyncActive()),
           folly::to<std::string>(statusEntry.get_rxPktSyncActive()),
           folly::to<std::string>(statusEntry.get_txPktSyncActive()),
           folly::to<std::string>(
               statusEntry.get_switchReachabilityChangeSyncActive())});
    }
    out << table << std::endl;
  }

  RetType createModel(
      std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus>& hwAgentStatus,
      MultiSwitchRunState& runStates) {
    RetType model;

    int switchIndex = 0;
    for (const auto& runState : *runStates.hwIndexToRunState()) {
      cli::HwAgentStatusEntry hwStatusEntry;
      hwStatusEntry.switchId() = runState.first;
      hwStatusEntry.switchIndex() = switchIndex;
      hwStatusEntry.runState() = getRunStateStr(runState.second);
      hwStatusEntry.linkSyncActive() =
          hwAgentStatus[switchIndex].get_linkEventSyncActive();
      hwStatusEntry.statsSyncActive() =
          hwAgentStatus[switchIndex].get_statsEventSyncActive();
      hwStatusEntry.fdbSyncActive() =
          hwAgentStatus[switchIndex].get_fdbEventSyncActive();
      hwStatusEntry.rxPktSyncActive() =
          hwAgentStatus[switchIndex].get_rxPktEventSyncActive();
      hwStatusEntry.txPktSyncActive() =
          hwAgentStatus[switchIndex].get_txPktEventSyncActive();
      hwStatusEntry.switchReachabilityChangeSyncActive() =
          hwAgentStatus[switchIndex]
              .get_switchReachabilityChangeEventSyncActive();
      model.hwAgentStatusEntries()->push_back(hwStatusEntry);
      switchIndex++;
    }
    return model;
  }

  std::string getRunStateStr(facebook::fboss::SwitchRunState runState) {
    switch (runState) {
      case SwitchRunState::UNINITIALIZED:
        return "UNINITIALIZED";
      case SwitchRunState::INITIALIZED:
        return "INITIALIZED";
      case SwitchRunState::CONFIGURED:
        return "CONFIGURED";
      case SwitchRunState::FIB_SYNCED:
        return "FIB_SYNCED";
      case SwitchRunState::EXITING:
        return "EXITING";
    }
    throw std::invalid_argument("Invalid Run State");
  }
};

} // namespace facebook::fboss
