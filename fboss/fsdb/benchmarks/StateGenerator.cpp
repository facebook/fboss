// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/benchmarks/StateGenerator.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

namespace facebook::fboss::fsdb::test {

void StateGenerator::fillVoqStats(
    AgentStats* stats,
    int nSysPorts,
    int nVoqsPerSysPort,
    int value) {
  stats->sysPortStatsMap()->emplace(0, std::map<std::string, HwSysPortStats>());
  for (int portId = 0; portId < nSysPorts; ++portId) {
    std::string portName = folly::to<std::string>(":eth/", portId, "/1");
    HwSysPortStats sysPortStatsEntry;
    for (int queueId = 0; queueId < nVoqsPerSysPort; queueId++) {
      sysPortStatsEntry.queueOutDiscardBytes_()[queueId] = value;
      sysPortStatsEntry.queueOutBytes_()[queueId] = value;
      sysPortStatsEntry.queueWatermarkBytes_()[queueId] = value;
      sysPortStatsEntry.queueWredDroppedPackets_()[queueId] = value;
      sysPortStatsEntry.queueCreditWatchdogDeletedPackets_()[queueId] = value;
      sysPortStatsEntry.queueLatencyWatermarkNsec_()[queueId] = value;
    }
    sysPortStatsEntry.portName_() = portName;
    sysPortStatsEntry.timestamp_() = 1;
    stats->sysPortStatsMap()->at(0).emplace(
        portName, std::move(sysPortStatsEntry));
  }
}

void StateGenerator::updateVoqStats(AgentStats* stats, int increment) {
  for (auto& [switchIdx, sysPortStatsMap] : *stats->sysPortStatsMap()) {
    for (auto& [portName, sysPortStatsEntry] : sysPortStatsMap) {
      for (int queueId = 0;
           queueId < sysPortStatsEntry.queueOutDiscardBytes_()->size();
           queueId++) {
        sysPortStatsEntry.queueOutDiscardBytes_()[queueId] += increment;
      }
      for (int queueId = 0;
           queueId < sysPortStatsEntry.queueOutBytes_()->size();
           queueId++) {
        sysPortStatsEntry.queueOutBytes_()[queueId] += increment;
      }
      for (int queueId = 0;
           queueId < sysPortStatsEntry.queueWatermarkBytes_()->size();
           queueId++) {
        sysPortStatsEntry.queueWatermarkBytes_()[queueId] += increment;
      }
      for (int queueId = 0;
           queueId < sysPortStatsEntry.queueWredDroppedPackets_()->size();
           queueId++) {
        sysPortStatsEntry.queueWredDroppedPackets_()[queueId] += increment;
      }
      for (int queueId = 0; queueId <
           sysPortStatsEntry.queueCreditWatchdogDeletedPackets_()->size();
           queueId++) {
        sysPortStatsEntry.queueCreditWatchdogDeletedPackets_()[queueId] +=
            increment;
      }
      for (int queueId = 0;
           queueId < sysPortStatsEntry.queueLatencyWatermarkNsec_()->size();
           queueId++) {
        sysPortStatsEntry.queueLatencyWatermarkNsec_()[queueId] += increment;
      }
    }
  }
}

} // namespace facebook::fboss::fsdb::test
