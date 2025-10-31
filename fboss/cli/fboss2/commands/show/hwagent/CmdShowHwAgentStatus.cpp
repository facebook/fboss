/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowHwAgentStatus.h"

#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/Conv.h"

#ifndef IS_OSS
#include "common/thrift/thrift/gen-cpp2/MonitorAsyncClient.h"
#endif

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowHwAgentStatusTraits::RetType;

void AgentCounters::getAgentCounters(
    HostInfo hostinfo,
    int numSwitches,
    SwHwAgentCounters& FBSwHwCounters) {
  /* Query Sw Agent for counters */
  std::map<std::string, int64_t> FBSwCounters;
  auto client =
      utils::createClient<apache::thrift::Client<facebook::fboss::FbossCtrl>>(
          hostinfo);
  client->sync_getCounters(FBSwCounters);

  /* Query Hw Agent for counters */
  std::vector<std::map<std::string, int64_t>> FBHwCountersVec;
  for (int i = 0; i < numSwitches; i++) {
    std::map<std::string, int64_t> FBHwCounters;
#ifndef IS_OSS
    try {
      auto hwClient =
          utils::createClient<apache::thrift::Client<FbossHwCtrl>>(hostinfo, i);
      apache::thrift::Client<facebook::thrift::Monitor> monitoringClient{
          hwClient->getChannelShared()};
      monitoringClient.sync_getCounters(FBHwCounters);
    } catch (const std::exception& ex) {
      XLOG(ERR) << ex.what();
    }
#endif
    FBHwCountersVec.push_back(FBHwCounters);
  }
  FBSwHwCounters.FBSwCounters = FBSwCounters;
  FBSwHwCounters.FBHwCountersVec = FBHwCountersVec;
  return;
}

RetType CmdShowHwAgentStatus::queryClient(const HostInfo& hostInfo) {
  std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus> hwAgentStatus;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getHwAgentConnectionStatus(hwAgentStatus);
  MultiSwitchRunState runState;
  client->sync_getMultiSwitchRunState(runState);
  auto numSwitches = static_cast<int>(runState.hwIndexToRunState()->size());
  struct SwHwAgentCounters swHwAgentCounters;
  counters_->getAgentCounters(hostInfo, numSwitches, swHwAgentCounters);
  return createModel(hwAgentStatus, runState, swHwAgentCounters);
}

void CmdShowHwAgentStatus::printOutput(
    const RetType& model,
    std::ostream& out) {
  std::vector<std::string> detailedOutput;

  out << "Output Format: <InSync/OutofSync - [Y]es/[N]o  | No. of Events sent | No. of Events received> "
      << std::endl;

  Table table;
  table.setHeader(
      {"SwitchIndex",
       "SwitchId",
       "State",
       "Link",
       "Stats",
       "Fdb",
       "RxPkt",
       "TxPkt",
       "SwitchReachability"});

  for (auto const& statusEntry : model.hwAgentStatusEntries().value()) {
    table.addRow(
        {folly::to<std::string>(folly::copy(statusEntry.switchIndex().value())),
         folly::to<std::string>(folly::copy(statusEntry.switchId().value())),
         statusEntry.runState().value(),
         folly::to<std::string>(
             statusEntry.linkSyncActive().value() ? "Y" : "N",
             "|",
             folly::copy(statusEntry.linkEventsSent().value()),
             "|",
             folly::copy(statusEntry.linkEventsReceived().value())),
         folly::to<std::string>(
             statusEntry.statsSyncActive().value() ? "Y" : "N",
             "|",
             folly::copy(statusEntry.HwSwitchStatsEventsSent().value()),
             "|",
             folly::copy(statusEntry.HwSwitchStatsEventsReceived().value())),
         folly::to<std::string>(
             statusEntry.fdbSyncActive().value() ? "Y" : "N",
             "|",
             folly::copy(statusEntry.fdbEventsSent().value()),
             "|",
             folly::copy(statusEntry.fdbEventsReceived().value())),
         folly::to<std::string>(
             statusEntry.rxPktSyncActive().value() ? "Y" : "N",
             "|",
             folly::copy(statusEntry.rxPktEventsSent().value()),
             "|",
             folly::copy(statusEntry.rxPktEventsReceived().value())),
         folly::to<std::string>(
             statusEntry.txPktSyncActive().value() ? "Y" : "N",
             "|",
             folly::copy(statusEntry.txPktEventsSent().value()),
             "|",
             folly::copy(statusEntry.txPktEventsReceived().value())),
         folly::to<std::string>(
             statusEntry.switchReachabilityChangeSyncActive().value() ? "Y"
                                                                      : "N",
             "|",
             folly::copy(
                 statusEntry.switchReachabilityChangeEventsSent().value()),
             "|",
             folly::copy(statusEntry.switchReachabilityChangeEventsReceived()
                             .value()))});
  }
  out << table << std::endl;
}

int64_t CmdShowHwAgentStatus::getCounterValue(
    const std::map<std::string, int64_t>& counters,
    int switchIndex,
    const std::string& counterName) {
  /* Test if the counter names have ".". If it still has ".." fallback to old
   * counter names
   * TODO: Remove this once we have migrated to new counter names with single
   * "."
   */
  if (counters.contains(
          folly::to<std::string>("switch.", switchIndex, ".", counterName))) {
    return counters.at(
        folly::to<std::string>("switch.", switchIndex, ".", counterName));
  } else if (counters.contains(
                 folly::to<std::string>(
                     "switch.", switchIndex, "..", counterName))) {
    return counters.at(
        folly::to<std::string>("switch.", switchIndex, "..", counterName));
  } else if (counters.contains(counterName)) {
    return counters.at(counterName);
  } else {
    return 0;
  }
}

RetType CmdShowHwAgentStatus::createModel(
    std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus>& hwAgentStatus,
    MultiSwitchRunState& runStates,
    const struct SwHwAgentCounters& FBSwHwCounters) {
  RetType model;

  int switchIndex = 0;
  for (const auto& runState : *runStates.hwIndexToRunState()) {
    cli::HwAgentStatusEntry hwStatusEntry;
    hwStatusEntry.switchId() = runState.first;
    hwStatusEntry.switchIndex() = switchIndex;
    hwStatusEntry.runState() = getRunStateStr(runState.second);
    hwStatusEntry.linkSyncActive() =
        folly::copy(hwAgentStatus[switchIndex].linkEventSyncActive().value());
    hwStatusEntry.statsSyncActive() =
        folly::copy(hwAgentStatus[switchIndex].statsEventSyncActive().value());
    hwStatusEntry.fdbSyncActive() =
        folly::copy(hwAgentStatus[switchIndex].fdbEventSyncActive().value());
    hwStatusEntry.rxPktSyncActive() =
        folly::copy(hwAgentStatus[switchIndex].rxPktEventSyncActive().value());
    hwStatusEntry.txPktSyncActive() =
        folly::copy(hwAgentStatus[switchIndex].txPktEventSyncActive().value());
    hwStatusEntry.switchReachabilityChangeSyncActive() = folly::copy(
        hwAgentStatus[switchIndex]
            .switchReachabilityChangeEventSyncActive()
            .value());
    hwStatusEntry.linkEventsReceived() = getCounterValue(
        FBSwHwCounters.FBSwCounters, switchIndex, "link_event_received.sum");
    hwStatusEntry.linkEventsSent() =
        (switchIndex < static_cast<int>(FBSwHwCounters.FBHwCountersVec.size()))
        ? getCounterValue(
              FBSwHwCounters.FBHwCountersVec[switchIndex],
              switchIndex,
              "LinkChangeEventThriftSyncer.events_sent.sum")
        : 0;
    hwStatusEntry.txPktEventsSent() = getCounterValue(
        FBSwHwCounters.FBSwCounters, switchIndex, "tx_pkt_event_sent.sum");
    hwStatusEntry.txPktEventsReceived() =
        (switchIndex < static_cast<int>(FBSwHwCounters.FBHwCountersVec.size()))
        ? getCounterValue(
              FBSwHwCounters.FBHwCountersVec[switchIndex],
              switchIndex,
              "TxPktEventThriftSyncer.events_received.sum")
        : 0;
    hwStatusEntry.rxPktEventsReceived() = getCounterValue(
        FBSwHwCounters.FBSwCounters, switchIndex, "rx_pkt_event_received.sum");
    hwStatusEntry.rxPktEventsSent() =
        (switchIndex < static_cast<int>(FBSwHwCounters.FBHwCountersVec.size()))
        ? getCounterValue(
              FBSwHwCounters.FBHwCountersVec[switchIndex],
              switchIndex,
              "RxPktEventThriftSyncer.events_sent.sum")
        : 0;
    hwStatusEntry.fdbEventsReceived() = getCounterValue(
        FBSwHwCounters.FBSwCounters, switchIndex, "fdb_event_received.sum");
    hwStatusEntry.fdbEventsSent() =
        (switchIndex < static_cast<int>(FBSwHwCounters.FBHwCountersVec.size()))
        ? getCounterValue(
              FBSwHwCounters.FBHwCountersVec[switchIndex],
              switchIndex,
              "FdbEventThriftSyncer.events_sent.sum")
        : 0;
    hwStatusEntry.HwSwitchStatsEventsReceived() = getCounterValue(
        FBSwHwCounters.FBSwCounters, switchIndex, "stats_event_received.sum");
    hwStatusEntry.HwSwitchStatsEventsSent() =
        (switchIndex < static_cast<int>(FBSwHwCounters.FBHwCountersVec.size()))
        ? getCounterValue(
              FBSwHwCounters.FBHwCountersVec[switchIndex],
              switchIndex,
              "HwSwitchStatsSinkClient.events_sent.sum")
        : 0;
    hwStatusEntry.switchReachabilityChangeEventsReceived() = getCounterValue(
        FBSwHwCounters.FBSwCounters,
        switchIndex,
        "switch_reachability_change_event_received.sum");
    hwStatusEntry.switchReachabilityChangeEventsSent() =
        (switchIndex < static_cast<int>(FBSwHwCounters.FBHwCountersVec.size()))
        ? getCounterValue(
              FBSwHwCounters.FBHwCountersVec[switchIndex],
              switchIndex,
              "SwitchReachabilityChangeEventThriftSyncer.events_sent.sum")
        : 0;
    model.hwAgentStatusEntries()->push_back(hwStatusEntry);
    switchIndex++;
  }
  return model;
}

std::string CmdShowHwAgentStatus::getRunStateStr(
    facebook::fboss::SwitchRunState runState) {
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

} // namespace facebook::fboss
