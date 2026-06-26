/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowSystemPort.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <algorithm>
#include <unordered_set>
#include <vector>
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fmt/format.h"
#include "folly/Conv.h"
#include "folly/String.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowSystemPortTraits::RetType;

namespace {

cli::SystemPortHwStatsEntry makeSystemPortHwStatsEntry(
    const HwSysPortStats& systemHwStatsEntry) {
  cli::SystemPortHwStatsEntry portStats;

  portStats.egressDiscardBytes() =
      systemHwStatsEntry.queueOutDiscardBytes_().value();
  portStats.egressOutBytes() = systemHwStatsEntry.queueOutBytes_().value();
  portStats.egressWatermarkBytes() =
      systemHwStatsEntry.queueWatermarkBytes_().value();
  return portStats;
}

const HwSysPortStats* findSystemPortHwStats(
    const std::map<std::string, HwSysPortStats>& systemportHwStats,
    const std::string& systemPortName,
    const cli::SystemPortEntry& systemPortDetails) {
  if (systemPortDetails.switchIndex().has_value()) {
    auto iter = systemportHwStats.find(
        folly::to<std::string>(
            "switch.", *systemPortDetails.switchIndex(), ".", systemPortName));
    if (iter != systemportHwStats.end()) {
      return &iter->second;
    }
  }

  // TODO(daiweix): Remove this fallback after D106556657 is rolled out to all
  // new fboss agent binaries deployed in the production network.
  auto iter = systemportHwStats.find(systemPortName);
  return iter == systemportHwStats.end() ? nullptr : &iter->second;
}

void appendSystemPortHwStats(
    std::vector<std::string>& detailedOutput,
    const cli::SystemPortHwStatsEntry& sysPortHwStats,
    int totalVoqCount) {
  const auto& discardBytesMap = sysPortHwStats.egressDiscardBytes().value();
  const auto& outBytesMap = sysPortHwStats.egressOutBytes().value();
  const auto& watermarkBytesMap = sysPortHwStats.egressWatermarkBytes().value();

  detailedOutput.emplace_back(fmt::format("    Queue Discard (Bytes)"));
  for (int voqIndex = 0; voqIndex < totalVoqCount; ++voqIndex) {
    auto iter = discardBytesMap.find(voqIndex);
    if (iter != discardBytesMap.end()) {
      detailedOutput.emplace_back(
          fmt::format("\tVoq {} \t\t {}", voqIndex, iter->second));
    }
  }
  detailedOutput.emplace_back(fmt::format("    Queue Egress (Bytes)"));
  for (int voqIndex = 0; voqIndex < totalVoqCount; ++voqIndex) {
    auto iter = outBytesMap.find(voqIndex);
    if (iter != outBytesMap.end()) {
      detailedOutput.emplace_back(
          fmt::format("\tVoq {}\t\t {}", voqIndex, iter->second));
    }
  }
  detailedOutput.emplace_back(fmt::format("    Queue Watermark (Bytes)"));
  for (int voqIndex = 0; voqIndex < totalVoqCount; ++voqIndex) {
    auto iter = watermarkBytesMap.find(voqIndex);
    if (iter != watermarkBytesMap.end()) {
      detailedOutput.emplace_back(
          fmt::format("\tVoq {}\t\t {}", voqIndex, iter->second));
    }
  }
}

} // namespace

RetType CmdShowSystemPort::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedSystemPorts) {
  std::map<int64_t, facebook::fboss::SystemPortThrift> systemportEntries;
  std::map<std::string, facebook::fboss::HwSysPortStats> systemportEntryStats;
  std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getSystemPorts(systemportEntries);
  client->sync_getSwitchIdToSwitchInfo(switchIdToSwitchInfo);
  auto opt = CmdGlobalOptions::getInstance();
  if (opt->isDetailed()) {
    client->sync_getSysPortStats(systemportEntryStats);
  }

  return createModel(
      systemportEntries,
      queriedSystemPorts.data(),
      systemportEntryStats,
      switchIdToSwitchInfo);
}

void CmdShowSystemPort::printOutput(const RetType& model, std::ostream& out) {
  std::vector<std::string> detailedOutput;
  auto opt = CmdGlobalOptions::getInstance();
  auto getSwitchIndexStr = [](const cli::SystemPortEntry& systemportInfo) {
    return systemportInfo.switchIndex().has_value()
        ? folly::to<std::string>(*systemportInfo.switchIndex())
        : "--";
  };

  if (opt->isDetailed()) {
    for (auto const& systemportInfo : model.sysPortEntries().value()) {
      detailedOutput.emplace_back("");
      detailedOutput.emplace_back(
          fmt::format("Name:         \t\t {}", systemportInfo.name().value()));
      detailedOutput.emplace_back(
          fmt::format(
              "ID:           \t\t {}",
              folly::to<std::string>(
                  folly::copy(systemportInfo.id().value()))));
      detailedOutput.emplace_back(
          fmt::format(
              "SwitchIndex:  \t\t {}", getSwitchIndexStr(systemportInfo)));
      detailedOutput.emplace_back(
          fmt::format("Speed:        \t\t {}", systemportInfo.speed().value()));
      detailedOutput.emplace_back(
          fmt::format(
              "QosPolicy:    \t\t {}", systemportInfo.qosPolicy().value()));
      detailedOutput.emplace_back(
          fmt::format(
              "CoreIndex:    \t\t {}",
              folly::to<std::string>(
                  folly::copy(systemportInfo.coreIndex().value()))));
      detailedOutput.emplace_back(
          fmt::format(
              "CorePortIndex:\t\t {}",
              folly::to<std::string>(
                  folly::copy(systemportInfo.corePortIndex().value()))));
      detailedOutput.emplace_back(
          fmt::format(
              "Voqs:         \t\t {}",
              folly::to<std::string>(
                  folly::copy(systemportInfo.numVoqs().value()))));

      int totalVoqCount = folly::copy(systemportInfo.numVoqs().value());
      appendSystemPortHwStats(
          detailedOutput, systemportInfo.hwPortStats().value(), totalVoqCount);
    }
    out << folly::join("\n", detailedOutput) << std::endl;
  } else {
    Table table;
    table.setHeader(
        {"ID",
         "Name",
         "SwitchIndex",
         "Speed",
         "NumVoqs",
         "QosPolicy",
         "CoreIndex",
         "CorePortIndex",
         "RemoteSystemPortType",
         "RemoteSystemPortLivenessStatus",
         "Scope"});

    for (auto const& systemportInfo : model.sysPortEntries().value()) {
      table.addRow(
          {folly::to<std::string>(folly::copy(systemportInfo.id().value())),
           systemportInfo.name().value(),
           getSwitchIndexStr(systemportInfo),
           systemportInfo.speed().value(),
           folly::to<std::string>(
               folly::copy(systemportInfo.numVoqs().value())),
           systemportInfo.qosPolicy().value(),
           folly::to<std::string>(
               folly::copy(systemportInfo.coreIndex().value())),
           folly::to<std::string>(
               folly::copy(systemportInfo.corePortIndex().value())),
           folly::to<std::string>(
               systemportInfo.remoteSystemPortType().value()),
           folly::to<std::string>(
               systemportInfo.remoteSystemPortLivenessStatus().value()),
           systemportInfo.scope().value()});
    }
    out << table << std::endl;
  }
}

RetType CmdShowSystemPort::createModel(
    std::map<int64_t, facebook::fboss::SystemPortThrift> systemPortEntries,
    const ObjectArgType& queriedSystemPorts,
    const std::map<std::string, facebook::fboss::HwSysPortStats>&
        systemportHwStats,
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo) {
  RetType model;
  std::unordered_set<std::string> queriedSet(
      queriedSystemPorts.begin(), queriedSystemPorts.end());

  for (const auto& entry : systemPortEntries) {
    auto systemPortInfo = entry.second;
    const auto& systemPortName = systemPortInfo.portName().value();

    if (queriedSystemPorts.size() == 0 || queriedSet.count(systemPortName)) {
      cli::SystemPortEntry systemPortDetails;
      systemPortDetails.id() = folly::copy(systemPortInfo.portId().value());
      systemPortDetails.name() = systemPortInfo.portName().value();
      systemPortDetails.speed() =
          utils::getSpeedGbps(folly::copy(systemPortInfo.speedMbps().value()));
      systemPortDetails.numVoqs() =
          folly::copy(systemPortInfo.numVoqs().value());
      systemPortDetails.qosPolicy() =
          (apache::thrift::get_pointer(systemPortInfo.qosPolicy())
               ? *apache::thrift::get_pointer(systemPortInfo.qosPolicy())
               : " -- ");
      systemPortDetails.coreIndex() =
          folly::copy(systemPortInfo.coreIndex().value());
      systemPortDetails.corePortIndex() =
          folly::copy(systemPortInfo.corePortIndex().value());

      auto getRemoteSystemPortTypeStr = [](const auto& remoteSystemPortType) {
        if (remoteSystemPortType.has_value()) {
          switch (remoteSystemPortType.value()) {
            case RemoteSystemPortType::DYNAMIC_ENTRY:
              return "DYNAMIC";
            case RemoteSystemPortType::STATIC_ENTRY:
              return "STATIC";
          }
        }
        return "--";
      };
      systemPortDetails.remoteSystemPortType() =
          getRemoteSystemPortTypeStr(systemPortInfo.remoteSystemPortType());

      auto getRemoteSystemPortLivenessStatusStr =
          [](const auto& remoteSystemPortLivenessStatus) {
            if (remoteSystemPortLivenessStatus.has_value()) {
              switch (remoteSystemPortLivenessStatus.value()) {
                case LivenessStatus::LIVE:
                  return "LIVE";
                case LivenessStatus::STALE:
                  return "STALE";
              }
            }
            return "--";
          };
      systemPortDetails.remoteSystemPortLivenessStatus() =
          getRemoteSystemPortLivenessStatusStr(
              systemPortInfo.remoteSystemPortLivenessStatus());

      systemPortDetails.scope() = apache::thrift::util::enumNameSafe(
          folly::copy(systemPortInfo.scope().value()));

      auto switchInfoIter =
          switchIdToSwitchInfo.find(*systemPortInfo.switchId());
      if (switchInfoIter != switchIdToSwitchInfo.end()) {
        systemPortDetails.switchIndex() = *switchInfoIter->second.switchIndex();
      }

      // see if we have any detailed hw stats
      if (const auto* sysPortStats = findSystemPortHwStats(
              systemportHwStats, systemPortName, systemPortDetails)) {
        systemPortDetails.hwPortStats() =
            makeSystemPortHwStatsEntry(*sysPortStats);
      }
      model.sysPortEntries()->push_back(systemPortDetails);
    }
  }

  std::sort(
      model.sysPortEntries()->begin(),
      model.sysPortEntries()->end(),
      [&](const cli::SystemPortEntry& a, const cli::SystemPortEntry& b) {
        return utils::compareSystemPortName(a.name().value(), b.name().value());
      });

  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowSystemPort, CmdShowSystemPortTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowSystemPort, CmdShowSystemPortTraits>::getValidFilters();

} // namespace facebook::fboss
