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

#include <folly/json/json.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <unistd.h>
#include <algorithm>

namespace facebook::fboss {

using utils::Table;

struct CmdShowPortTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = utils::PortList;
  using RetType = cli::ShowPortModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowPort : public CmdHandler<CmdShowPort, CmdShowPortTraits> {
 public:
  using ObjectArgType = CmdShowPortTraits::ObjectArgType;
  using RetType = CmdShowPortTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts) {
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverEntries;
    std::map<std::string, facebook::fboss::HwPortStats> portStats;

    std::vector<int32_t> requiredTransceiverEntries;
    std::vector<std::string> bgpDrainedInterfaces;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    client->sync_getAllPortInfo(portEntries);

    auto opt = CmdGlobalOptions::getInstance();
    if (opt->isDetailed()) {
      client->sync_getHwPortStats(portStats);
    }

    try {
      auto qsfpService =
          utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(
              hostInfo);

      qsfpService->sync_getTransceiverInfo(
          transceiverEntries, requiredTransceiverEntries);
    } catch (apache::thrift::transport::TTransportException&) {
      std::cerr << "Cannot connect to qsfp_service\n";
    }

    return createModel(
        portEntries,
        transceiverEntries,
        queriedPorts.data(),
        portStats,
        utils::getBgpDrainedInterafces(hostInfo));
  }

  std::unordered_map<std::string, std::vector<std::string>>
  getAcceptedFilterValues() {
    return {
        {"adminState", {"Enabled", "Disabled"}},
        {"linkState", {"Up", "Down"}},
        {"activeState", {"Active", "Inactive", "--"}},
    };
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    std::vector<std::string> detailedOutput;
    auto opt = CmdGlobalOptions::getInstance();

    if (opt->isDetailed()) {
      for (auto const& portInfo : model.get_portEntries()) {
        std::string hwLogicalPortId;
        if (auto portId = portInfo.hwLogicalPortId()) {
          hwLogicalPortId = folly::to<std::string>(*portId);
        }

        const auto& portHwStats = portInfo.get_hwPortStats();
        detailedOutput.emplace_back("");
        detailedOutput.emplace_back(
            fmt::format("Name:           \t\t {}", portInfo.get_name()));
        detailedOutput.emplace_back(fmt::format(
            "ID:             \t\t {}",
            folly::to<std::string>(portInfo.get_id())));
        detailedOutput.emplace_back(
            fmt::format("Admin State:    \t\t {}", portInfo.get_adminState()));
        detailedOutput.emplace_back(
            fmt::format("Speed:          \t\t {}", portInfo.get_speed()));
        detailedOutput.emplace_back(
            fmt::format("LinkState:      \t\t {}", portInfo.get_linkState()));
        detailedOutput.emplace_back(fmt::format(
            "TcvrID:         \t\t {}",
            folly::to<std::string>(portInfo.get_tcvrID())));
        detailedOutput.emplace_back(
            fmt::format("Transceiver:    \t\t {}", portInfo.get_tcvrPresent()));
        detailedOutput.emplace_back(
            fmt::format("ProfileID:      \t\t {}", portInfo.get_profileId()));
        detailedOutput.emplace_back(
            fmt::format("ProfileID:      \t\t {}", hwLogicalPortId));
        detailedOutput.emplace_back(
            fmt::format("Core ID:             \t\t {}", portInfo.get_coreId()));
        detailedOutput.emplace_back(fmt::format(
            "Virtual device ID:    \t\t {}", portInfo.get_virtualDeviceId()));
        if (portInfo.get_pause()) {
          detailedOutput.emplace_back(
              fmt::format("Pause:          \t\t {}", *portInfo.get_pause()));
        } else if (portInfo.get_pfc()) {
          detailedOutput.emplace_back(
              fmt::format("PFC:            \t\t {}", *portInfo.get_pfc()));
        }
        detailedOutput.emplace_back(fmt::format(
            "Unicast queues: \t\t {}",
            folly::to<std::string>(portInfo.get_numUnicastQueues())));
        detailedOutput.emplace_back(fmt::format(
            "    Ingress (bytes)               \t\t {}",
            portHwStats.get_ingressBytes()));
        detailedOutput.emplace_back(fmt::format(
            "    Egress (bytes)                \t\t {}",
            portHwStats.get_egressBytes()));
        for (const auto& queueBytes : portHwStats.get_queueOutBytes()) {
          const auto iter = portInfo.get_queueIdToName().find(queueBytes.first);
          std::string queueName = "";
          if (iter != portInfo.get_queueIdToName().end()) {
            queueName = folly::to<std::string>("(", iter->second, ")");
          }
          // print either if the queue is valid or queue has non zero traffic
          if (queueBytes.second || !queueName.empty()) {
            detailedOutput.emplace_back(fmt::format(
                "\tQueue {} {:12}    \t\t {}",
                queueBytes.first,
                queueName,
                queueBytes.second));
          }
        }
        detailedOutput.emplace_back(fmt::format(
            "    Received Unicast (pkts)       \t\t {}",
            portHwStats.get_inUnicastPkts()));
        detailedOutput.emplace_back(fmt::format(
            "    In Errors (pkts)              \t\t {}",
            portHwStats.get_inErrorPkts()));
        detailedOutput.emplace_back(fmt::format(
            "    In Discards (pkts)            \t\t {}",
            portHwStats.get_inDiscardPkts()));
        detailedOutput.emplace_back(fmt::format(
            "    Out Discards (pkts)           \t\t {}",
            portHwStats.get_outDiscardPkts()));
        detailedOutput.emplace_back(fmt::format(
            "    Out Congestion Discards (pkts)\t\t {}",
            portHwStats.get_outCongestionDiscardPkts()));
        for (const auto& queueDiscardBytes :
             portHwStats.get_queueOutDiscardBytes()) {
          const auto iter =
              portInfo.get_queueIdToName().find(queueDiscardBytes.first);
          std::string queueName = "";
          if (iter != portInfo.get_queueIdToName().end()) {
            queueName = folly::to<std::string>("(", iter->second, ")");
          }
          // print either if the queue is valid or queue has non zero traffic
          if (queueDiscardBytes.second || !queueName.empty()) {
            detailedOutput.emplace_back(fmt::format(
                "\tQueue {} {:12}    \t\t {}",
                queueDiscardBytes.first,
                queueName,
                queueDiscardBytes.second));
          }
        }
        detailedOutput.emplace_back(fmt::format(
            "    In Congestion Discards (pkts)\t\t {}",
            portHwStats.get_inCongestionDiscards()));
        if (portHwStats.get_outPfcPackets()) {
          detailedOutput.emplace_back(fmt::format(
              "    PFC Output (pkts)             \t\t {}",
              *portHwStats.get_outPfcPackets()));
          if (portHwStats.get_outPfcPriorityPackets()) {
            for (const auto& pfcPriortyCounter :
                 *portHwStats.get_outPfcPriorityPackets()) {
              detailedOutput.emplace_back(fmt::format(
                  "\tPriority {}                 \t\t {}",
                  pfcPriortyCounter.first,
                  pfcPriortyCounter.second));
            }
          }
        }
        if (portHwStats.get_inPfcPackets()) {
          detailedOutput.emplace_back(fmt::format(
              "    PFC Input (pkts)              \t\t {}",
              *portHwStats.get_inPfcPackets()));
          if (portHwStats.get_inPfcPriorityPackets()) {
            for (const auto& pfcPriortyCounter :
                 *portHwStats.get_inPfcPriorityPackets()) {
              detailedOutput.emplace_back(fmt::format(
                  "\tPriority {}                 \t\t {}",
                  pfcPriortyCounter.first,
                  pfcPriortyCounter.second));
            }
          }
        }
        if (portHwStats.get_outPausePackets()) {
          detailedOutput.emplace_back(fmt::format(
              "    Pause Output (pkts)           \t\t {}",
              *portHwStats.get_outPausePackets()));
        }
        if (portHwStats.get_inPausePackets()) {
          detailedOutput.emplace_back(fmt::format(
              "    Pause Input (pkts)            \t\t {}",
              *portHwStats.get_inPausePackets()));
        }
      }
      out << folly::join("\n", detailedOutput) << std::endl;
    } else {
      Table table;
      table.setHeader({
          "ID",
          "Name",
          "AdminState",
          "LinkState",
          "ActiveState",
          "Transceiver",
          "TcvrID",
          "Speed",
          "ProfileID",
          "HwLogicalPortId",
          "Drained",
          "Errors",
          "Core Id",
          "Virtual device Id",
      });

      for (auto const& portInfo : model.get_portEntries()) {
        std::string hwLogicalPortId;
        if (auto portId = portInfo.hwLogicalPortId()) {
          hwLogicalPortId = folly::to<std::string>(*portId);
        }
        table.addRow(
            {folly::to<std::string>(portInfo.get_id()),
             portInfo.get_name(),
             portInfo.get_adminState(),
             getStyledLinkState(portInfo.get_linkState()),
             getStyledActiveState(portInfo.get_activeState()),
             portInfo.get_tcvrPresent(),
             folly::to<std::string>(portInfo.get_tcvrID()),
             portInfo.get_speed(),
             portInfo.get_profileId(),
             hwLogicalPortId,
             portInfo.get_isDrained(),
             getStyledErrors(portInfo.get_activeErrors()),
             portInfo.get_coreId(),
             portInfo.get_virtualDeviceId()});
      }
      out << table << std::endl;
    }
  }

  std::string getAdminStateStr(PortAdminState adminState) {
    switch (adminState) {
      case PortAdminState::DISABLED:
        return "Disabled";
      case PortAdminState::ENABLED:
        return "Enabled";
    }

    throw std::runtime_error(
        "Unsupported AdminState: " +
        std::to_string(static_cast<int>(adminState)));
  }
  std::string getOptionalIntStr(std::optional<int> val) {
    if (val.has_value()) {
      return folly::to<std::string>(*val);
    }
    return "--";
  }

  Table::StyledCell getStyledLinkState(std::string linkState) {
    if (linkState == "Down") {
      return Table::StyledCell("Down", Table::Style::ERROR);
    } else {
      return Table::StyledCell("Up", Table::Style::GOOD);
    }

    throw std::runtime_error("Unsupported LinkState: " + linkState);
  }

  std::string getOperStateStr(PortOperState operState) {
    switch (operState) {
      case PortOperState::DOWN:
        return "Down";
      case PortOperState::UP:
        return "Up";
    }

    throw std::runtime_error(
        "Unsupported LinkState: " +
        std::to_string(static_cast<int>(operState)));
  }

  Table::StyledCell getStyledActiveState(std::string activeState) {
    if (activeState == "Inactive") {
      return Table::StyledCell("Inactive", Table::Style::ERROR);
    } else if (activeState == "Active") {
      return Table::StyledCell("Active", Table::Style::GOOD);
    } else {
      return Table::StyledCell("--", Table::Style::NONE);
    }

    throw std::runtime_error("Unsupported ActiveState: " + activeState);
  }

  std::string getActiveStateStr(PortActiveState* activeState) {
    if (activeState) {
      switch (*activeState) {
        case PortActiveState::INACTIVE:
          return "Inactive";
        case PortActiveState::ACTIVE:
          return "Active";
      }

      throw std::runtime_error(
          "Unsupported ActiveState: " +
          std::to_string(static_cast<int>(*activeState)));
    } else {
      return "--";
    }
  }

  std::string getTransceiverStr(
      std::map<int32_t, facebook::fboss::TransceiverInfo>& transceiverEntries,
      int32_t transceiverId) {
    if (transceiverEntries.count(transceiverId) == 0) {
      return "";
    }
    auto isPresent = *transceiverEntries[transceiverId].tcvrState()->present();
    if (isPresent)
      return "Present";
    return "Absent";
  }
  Table::StyledCell getStyledErrors(std::string errors) {
    if (errors == "--") {
      return Table::StyledCell(errors, Table::Style::NONE);
    } else {
      return Table::StyledCell(errors, Table::Style::ERROR);
    }
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
      std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverEntries,
      const ObjectArgType& queriedPorts,
      std::map<std::string, facebook::fboss::HwPortStats> portStats,
      const std::vector<std::string>& drainedInterfaces) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedPorts.begin(), queriedPorts.end());

    for (const auto& entry : portEntries) {
      auto portInfo = entry.second;
      auto portName = portInfo.get_name();
      auto operState = getOperStateStr(portInfo.get_operState());
      auto activeState = getActiveStateStr(portInfo.get_activeState());

      if (queriedPorts.size() == 0 || queriedSet.count(portName)) {
        cli::PortEntry portDetails;
        portDetails.id() = portInfo.get_portId();
        portDetails.name() = portInfo.get_name();
        portDetails.adminState() = getAdminStateStr(portInfo.get_adminState());
        portDetails.linkState() = operState;
        portDetails.activeState() = activeState;
        portDetails.speed() = utils::getSpeedGbps(portInfo.get_speedMbps());
        portDetails.profileId() = portInfo.get_profileID();
        portDetails.coreId() =
            getOptionalIntStr(portInfo.coreId().to_optional());
        portDetails.virtualDeviceId() =
            getOptionalIntStr(portInfo.virtualDeviceId().to_optional());
        if (portInfo.activeErrors()->size()) {
          std::vector<std::string> errorStrs;
          std::for_each(
              portInfo.activeErrors()->begin(),
              portInfo.activeErrors()->end(),
              [&errorStrs](auto error) {
                errorStrs.push_back(apache::thrift::util::enumNameSafe(error));
              });
          portDetails.activeErrors() = folly::join(",", errorStrs);
        } else {
          portDetails.activeErrors() = "--";
        }
        if (auto hwLogicalPortId = portInfo.hwLogicalPortId()) {
          portDetails.hwLogicalPortId() = *hwLogicalPortId;
        }
        portDetails.isDrained() = "No";
        if ((std::find(
                 drainedInterfaces.begin(),
                 drainedInterfaces.end(),
                 portName) != drainedInterfaces.end()) ||
            portInfo.get_isDrained()) {
          portDetails.isDrained() = "Yes";
        }
        if (auto tcvrId = portInfo.transceiverIdx()) {
          const auto transceiverId = tcvrId->get_transceiverId();
          portDetails.tcvrID() = transceiverId;
          portDetails.tcvrPresent() =
              getTransceiverStr(transceiverEntries, transceiverId);
        }
        if (auto pfc = portInfo.get_pfc()) {
          std::string pfcString = "";
          if (*pfc->tx()) {
            pfcString = "TX ";
          }
          if (*pfc->rx()) {
            pfcString += "RX ";
          }
          if (*pfc->watchdog()) {
            pfcString += "WD";
          }
          portDetails.pfc() = pfcString;
        } else {
          std::string pauseString = "";
          if (portInfo.get_txPause()) {
            pauseString = "TX ";
          }
          if (portInfo.get_rxPause()) {
            pauseString += "RX";
          }
          portDetails.pause() = pauseString;
        }
        portDetails.numUnicastQueues() = portInfo.get_portQueues().size();
        for (const auto& queue : portInfo.get_portQueues()) {
          if (!queue.get_name().empty()) {
            portDetails.queueIdToName()->insert(
                {queue.get_id(), queue.get_name()});
          }
        }

        const auto& iter = portStats.find(portName);
        if (iter != portStats.end()) {
          auto portHwStatsEntry = iter->second;
          cli::PortHwStatsEntry cliPortStats;
          cliPortStats.inUnicastPkts() = portHwStatsEntry.get_inUnicastPkts_();
          cliPortStats.inDiscardPkts() = portHwStatsEntry.get_inDiscards_();
          cliPortStats.inErrorPkts() = portHwStatsEntry.get_inErrors_();
          cliPortStats.outDiscardPkts() = portHwStatsEntry.get_outDiscards_();
          cliPortStats.outCongestionDiscardPkts() =
              portHwStatsEntry.get_outCongestionDiscardPkts_();
          cliPortStats.queueOutDiscardBytes() =
              portHwStatsEntry.get_queueOutDiscardBytes_();
          cliPortStats.inCongestionDiscards() =
              portHwStatsEntry.get_inCongestionDiscards_();
          cliPortStats.queueOutBytes() = portHwStatsEntry.get_queueOutBytes_();
          if (portInfo.get_pfc()) {
            cliPortStats.outPfcPriorityPackets() =
                portHwStatsEntry.get_outPfc_();
            cliPortStats.inPfcPriorityPackets() = portHwStatsEntry.get_inPfc_();
            cliPortStats.outPfcPackets() = portHwStatsEntry.get_outPfcCtrl_();
            cliPortStats.inPfcPackets() = portHwStatsEntry.get_inPfcCtrl_();
          } else {
            cliPortStats.outPausePackets() = portHwStatsEntry.get_outPause_();
            cliPortStats.inPausePackets() = portHwStatsEntry.get_inPause_();
          }
          cliPortStats.ingressBytes() = portHwStatsEntry.get_inBytes_();
          cliPortStats.egressBytes() = portHwStatsEntry.get_outBytes_();
          portDetails.hwPortStats() = cliPortStats;
        }
        model.portEntries()->push_back(portDetails);
      }
    }

    std::sort(
        model.portEntries()->begin(),
        model.portEntries()->end(),
        [&](const cli::PortEntry& a, const cli::PortEntry& b) {
          return utils::comparePortName(a.get_name(), b.get_name());
        });

    return model;
  }
};

} // namespace facebook::fboss
