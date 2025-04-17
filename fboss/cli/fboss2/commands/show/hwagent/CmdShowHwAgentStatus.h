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

    for (auto const& statusEntry : model.hwAgentStatusEntries().value()) {
      table.addRow(
          {folly::to<std::string>(
               folly::copy(statusEntry.switchIndex().value())),
           folly::to<std::string>(folly::copy(statusEntry.switchId().value())),
           statusEntry.runState().value(),
           folly::to<std::string>(
               folly::copy(statusEntry.linkSyncActive().value())),
           folly::to<std::string>(
               folly::copy(statusEntry.statsSyncActive().value())),
           folly::to<std::string>(
               folly::copy(statusEntry.fdbSyncActive().value())),
           folly::to<std::string>(
               folly::copy(statusEntry.rxPktSyncActive().value())),
           folly::to<std::string>(
               folly::copy(statusEntry.txPktSyncActive().value())),
           folly::to<std::string>(folly::copy(
               statusEntry.switchReachabilityChangeSyncActive().value()))});
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
          folly::copy(hwAgentStatus[switchIndex].linkEventSyncActive().value());
      hwStatusEntry.statsSyncActive() = folly::copy(
          hwAgentStatus[switchIndex].statsEventSyncActive().value());
      hwStatusEntry.fdbSyncActive() =
          folly::copy(hwAgentStatus[switchIndex].fdbEventSyncActive().value());
      hwStatusEntry.rxPktSyncActive() = folly::copy(
          hwAgentStatus[switchIndex].rxPktEventSyncActive().value());
      hwStatusEntry.txPktSyncActive() = folly::copy(
          hwAgentStatus[switchIndex].txPktEventSyncActive().value());
      hwStatusEntry.switchReachabilityChangeSyncActive() =
          folly::copy(hwAgentStatus[switchIndex]
                          .switchReachabilityChangeEventSyncActive()
                          .value());
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
