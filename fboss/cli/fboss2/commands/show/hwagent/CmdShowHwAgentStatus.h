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
#ifndef IS_OSS
#include "common/thrift/thrift/gen-cpp2/MonitorAsyncClient.h"
#endif
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/hwagent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <unistd.h>
#include <algorithm>

namespace facebook::fboss {

using utils::Table;

struct CmdShowHwAgentStatusTraits : public ReadCommandTraits {
  using RetType = cli::ShowHwAgentStatusModel;
};

struct SwHwAgentCounters {
  std::map<std::string, int64_t> FBSwCounters;
  std::vector<std::map<std::string, int64_t>> FBHwCountersVec;
};

/* Interface defined for mocking getAgentCounters in gtest*/
class AgentCountersIf {
 public:
  virtual ~AgentCountersIf() = default;
  virtual void getAgentCounters(HostInfo, int, SwHwAgentCounters&) = 0;
};

class AgentCounters : public AgentCountersIf {
 public:
  void getAgentCounters(
      HostInfo hostinfo,
      int numSwitches,
      SwHwAgentCounters& FBSwHwCounters) override {
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
            utils::createClient<apache::thrift::Client<FbossHwCtrl>>(
                hostinfo, i);
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
};

class CmdShowHwAgentStatus
    : public CmdHandler<CmdShowHwAgentStatus, CmdShowHwAgentStatusTraits> {
  std::unique_ptr<AgentCounters> defaultCounters_;
  AgentCountersIf* counters_;

 public:
  using RetType = CmdShowHwAgentStatusTraits::RetType;
  CmdShowHwAgentStatus()
      : defaultCounters_(std::make_unique<AgentCounters>()),
        counters_(defaultCounters_.get()) {}
  CmdShowHwAgentStatus(AgentCountersIf* counters) : counters_(counters) {}
  RetType queryClient(const HostInfo& hostInfo) {
    std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus> hwAgentStatus;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getHwAgentConnectionStatus(hwAgentStatus);
    MultiSwitchRunState runState;
    client->sync_getMultiSwitchRunState(runState);
    auto numSwitches = runState.hwIndexToRunState()->size();
    struct SwHwAgentCounters swHwAgentCounters;
    counters_->getAgentCounters(hostInfo, numSwitches, swHwAgentCounters);
    return createModel(hwAgentStatus, runState, swHwAgentCounters);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
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
          {folly::to<std::string>(
               folly::copy(statusEntry.switchIndex().value())),
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

  int64_t getCounterValue(
      std::map<std::string, int64_t> counters,
      int switchIndex,
      const std::string& counterName) {
    /* Test if the counter names have ".". If it still has ".." fallback to old
     * counter names
     * TODO: Remove this once we have migrated to new counter names with single
     * "."
     */
    if (counters.contains(
            folly::to<std::string>("switch.", switchIndex, ".", counterName))) {
      return counters[folly::to<std::string>(
          "switch.", switchIndex, ".", counterName)];
    } else if (counters.contains(folly::to<std::string>(
                   "switch.", switchIndex, "..", counterName))) {
      return counters[folly::to<std::string>(
          "switch.", switchIndex, "..", counterName)];
    } else if (counters.contains(counterName)) {
      return counters[counterName];
    } else {
      return 0;
    }
  }

  RetType createModel(
      std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus>& hwAgentStatus,
      MultiSwitchRunState& runStates,
      struct SwHwAgentCounters FBSwHwCounters) {
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
      hwStatusEntry.linkEventsReceived() = getCounterValue(
          FBSwHwCounters.FBSwCounters, switchIndex, "link_event_received.sum");
      hwStatusEntry.linkEventsSent() = getCounterValue(
          FBSwHwCounters.FBHwCountersVec[switchIndex],
          switchIndex,
          "LinkChangeEventThriftSyncer.events_sent.sum");
      hwStatusEntry.txPktEventsSent() = getCounterValue(
          FBSwHwCounters.FBSwCounters, switchIndex, "tx_pkt_event_sent.sum");
      hwStatusEntry.txPktEventsReceived() = getCounterValue(
          FBSwHwCounters.FBHwCountersVec[switchIndex],
          switchIndex,
          "TxPktEventThriftSyncer.events_received.sum");
      hwStatusEntry.rxPktEventsReceived() = getCounterValue(
          FBSwHwCounters.FBSwCounters,
          switchIndex,
          "rx_pkt_event_received.sum");
      hwStatusEntry.rxPktEventsSent() = getCounterValue(
          FBSwHwCounters.FBHwCountersVec[switchIndex],
          switchIndex,
          "RxPktEventThriftSyncer.events_sent.sum");
      hwStatusEntry.fdbEventsReceived() = getCounterValue(
          FBSwHwCounters.FBSwCounters, switchIndex, "fdb_event_received.sum");
      hwStatusEntry.fdbEventsSent() = getCounterValue(
          FBSwHwCounters.FBHwCountersVec[switchIndex],
          switchIndex,
          "FdbEventThriftSyncer.events_sent.sum");
      hwStatusEntry.HwSwitchStatsEventsReceived() = getCounterValue(
          FBSwHwCounters.FBSwCounters, switchIndex, "stats_event_received.sum");
      hwStatusEntry.HwSwitchStatsEventsSent() = getCounterValue(
          FBSwHwCounters.FBHwCountersVec[switchIndex],
          switchIndex,
          "HwSwitchStatsSinkClient.events_sent.sum");
      hwStatusEntry.switchReachabilityChangeEventsReceived() = getCounterValue(
          FBSwHwCounters.FBSwCounters,
          switchIndex,
          "switch_reachability_change_event_received.sum");
      hwStatusEntry.switchReachabilityChangeEventsSent() = getCounterValue(
          FBSwHwCounters.FBHwCountersVec[switchIndex],
          switchIndex,
          "SwitchReachabilityChangeEventThriftSyncer.events_sent.sum");
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
