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

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/cli/fboss2/commands/show/transceiver/gen-cpp2/model_types.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/transceiver/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"

#include <fmt/core.h>
#include <cstdint>

namespace facebook::fboss {

using utils::Table;

struct CmdShowTransceiverTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowTransceiverModel;
};

class CmdShowTransceiver
    : public CmdHandler<CmdShowTransceiver, CmdShowTransceiverTraits> {
 public:
  using ObjectArgType = CmdShowTransceiver::ObjectArgType;
  using RetType = CmdShowTransceiver::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts) {
    auto qsfpService = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
    auto agent = utils::createClient<FbossCtrlAsyncClient>(hostInfo);

    // TODO: explore performance improvement if we make all this parallel.
    auto portEntries = queryPortInfo(agent.get(), queriedPorts);
    auto portStatusEntries = queryPortStatus(agent.get(), portEntries);
    auto transceiverEntries =
        queryTransceiverInfo(qsfpService.get(), portStatusEntries);

    return createModel(portStatusEntries, transceiverEntries, portEntries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table outTable;

    outTable.setHeader(
        {"Interface",
         "Status",
         "Present",
         "Vendor",
         "Serial",
         "Part Number",
         "Temperature (C)",
         "Voltage (V)",
         "Current (mA)",
         "Tx Power (dBm)",
         "Rx Power (dBm)",
         "Rx SNR"});

    for (const auto& [portId, details] : model.get_transceivers()) {
      outTable.addRow({
          details.get_name(),
          (details.get_isUp()) ? "Up" : "Down",
          (details.get_isPresent()) ? "Present" : "Absent",
          details.get_vendor(),
          details.get_serial(),
          details.get_partNumber(),
          fmt::format("{:.2f}", details.get_temperature()),
          fmt::format("{:.2f}", details.get_voltage()),
          listToString(
              details.get_currentMA(), LOW_CURRENT_WARN, LOW_CURRENT_ERR),
          listToString(details.get_txPower(), LOW_POWER_WARN, LOW_POWER_ERR),
          listToString(details.get_rxPower(), LOW_POWER_WARN, LOW_POWER_ERR),
          listToString(details.get_rxSnr(), LOW_SNR_WARN, LOW_SNR_ERR),
      });
    }
    out << outTable << std::endl;
  }

 private:
  // These power thresholds are ported directly from fb_toolkit.  Eventually
  // they will be codified into a DNTT thrift service and we will be able to
  // query threshold specs instead of hardcoding.
  const double LOW_CURRENT_ERR = 0.00;
  const double LOW_CURRENT_WARN = 6.00;
  const double LOW_POWER_ERR = -9.00;
  const double LOW_POWER_WARN = -7.00;
  const double LOW_SNR_ERR = 19.00;
  const double LOW_SNR_WARN = 20.00;

  std::map<int, PortInfoThrift> queryPortInfo(
      FbossCtrlAsyncClient* agent,
      const ObjectArgType& queriedPorts) const {
    std::map<int, PortInfoThrift> portEntries;
    agent->sync_getAllPortInfo(portEntries);
    if (queriedPorts.size() == 0) {
      return portEntries;
    }

    std::map<int, PortInfoThrift> filteredPortEntries;
    for (auto const& [portId, portInfo] : portEntries) {
      for (auto const& queriedPort : queriedPorts) {
        if (portInfo.get_name() == queriedPort) {
          filteredPortEntries.emplace(portId, portInfo);
        }
      }
    }
    return filteredPortEntries;
  }

  Table::StyledCell listToString(
      std::vector<double> listToPrint,
      double lowWarningThreshold,
      double lowErrorThreshold) {
    std::string result;
    Table::Style cellStyle = Table::Style::GOOD;
    for (size_t i = 0; i < listToPrint.size(); ++i) {
      result += fmt::format("{:.2f}", listToPrint[i]) +
          ((i != listToPrint.size() - 1) ? "," : "");

      if (listToPrint[i] <= lowWarningThreshold) {
        cellStyle = Table::Style::WARN;
      }
      if (listToPrint[i] <= lowErrorThreshold) {
        cellStyle = Table::Style::ERROR;
      }
    }
    return Table::StyledCell(result, cellStyle);
  }

  std::map<int, PortStatus> queryPortStatus(
      FbossCtrlAsyncClient* agent,
      std::map<int, PortInfoThrift> portEntries) const {
    std::vector<int32_t> requiredPortStatus;
    for (const auto& portItr : portEntries) {
      requiredPortStatus.push_back(portItr.first);
    }
    std::map<int, PortStatus> portStatusEntries;
    agent->sync_getPortStatus(portStatusEntries, requiredPortStatus);

    return portStatusEntries;
  }

  std::map<int, TransceiverInfo> queryTransceiverInfo(
      QsfpServiceAsyncClient* qsfpService,
      std::map<int, PortStatus> portStatusEntries) const {
    std::vector<int32_t> requiredTransceiverEntries;
    for (const auto& portStatusItr : portStatusEntries) {
      if (auto tidx = portStatusItr.second.transceiverIdx()) {
        requiredTransceiverEntries.push_back(tidx->get_transceiverId());
      }
    }

    std::map<int, TransceiverInfo> transceiverEntries;
    qsfpService->sync_getTransceiverInfo(
        transceiverEntries, requiredTransceiverEntries);
    return transceiverEntries;
  }

  RetType createModel(
      std::map<int, PortStatus> portStatusEntries,
      std::map<int, TransceiverInfo> transceiverEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries) const {
    RetType model;

    // TODO: sort here?
    for (const auto& [portId, portEntry] : portStatusEntries) {
      cli::TransceiverDetail details;
      details.name() = portEntries[portId].get_name();
      if (!portEntry.transceiverIdx().has_value()) {
        // No transceiver information for this port. Skip printing
        continue;
      }
      const auto transceiverId =
          portEntry.transceiverIdx()->get_transceiverId();
      const auto& transceiver = transceiverEntries[transceiverId];
      const auto& tcvrState = *transceiver.tcvrState();
      const auto& tcvrStats = *transceiver.tcvrStats();
      details.isUp() = portEntry.get_up();
      details.isPresent() = tcvrState.get_present();
      if (const auto& vendor = tcvrState.vendor()) {
        details.vendor() = vendor->get_name();
        details.serial() = vendor->get_serialNumber();
        details.partNumber() = vendor->get_partNumber();
        details.temperature() = tcvrStats.get_sensor()->get_temp().get_value();
        details.voltage() = tcvrStats.get_sensor()->get_vcc().get_value();

        std::vector<double> current;
        std::vector<double> txPower;
        std::vector<double> rxPower;
        std::vector<double> rxSnr;
        for (const auto& channel : tcvrStats.get_channels()) {
          // Check if the transceiverInfo is updated to have the
          // portNameToMediaLanes field
          if (tcvrStats.portNameToMediaLanes().is_set()) {
            // Need to filter out the lanes for this specific port
            auto portToLaneMapIt = tcvrStats.portNameToMediaLanes()->find(
                portEntries[portId].get_name());
            // If the port in question exists in the map and if the
            // channel number is not in the list, skip printing information
            // about this channel
            if (portToLaneMapIt != tcvrStats.portNameToMediaLanes()->end() &&
                std::find(
                    portToLaneMapIt->second.begin(),
                    portToLaneMapIt->second.end(),
                    channel.get_channel()) == portToLaneMapIt->second.end()) {
              continue;
            }
            // If the port doesn't exist in the map, it's likely not configured
            // yet. Display all channels in this case
          }
          current.push_back(channel.get_sensors().get_txBias().get_value());
          txPower.push_back(channel.get_sensors().get_txPwrdBm()->get_value());
          rxPower.push_back(channel.get_sensors().get_rxPwrdBm()->get_value());
          if (const auto& snr = channel.get_sensors().rxSnr()) {
            rxSnr.push_back(snr->get_value());
          }
        }
        details.currentMA() = current;
        details.txPower() = txPower;
        details.rxPower() = rxPower;
        details.rxSnr() = rxSnr;
      }
      model.transceivers()->emplace(portId, std::move(details));
    }

    return model;
  }
};

} // namespace facebook::fboss
