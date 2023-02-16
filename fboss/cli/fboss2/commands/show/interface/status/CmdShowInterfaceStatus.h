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

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/status/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <cstdint>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceStatusTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowIntStatusModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceStatus
    : public CmdHandler<CmdShowInterfaceStatus, CmdShowInterfaceStatusTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceStatusTraits::ObjectArgType;
  using RetType = CmdShowInterfaceStatusTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);

    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverInfo;

    client->sync_getAllPortInfo(portEntries);
    qsfpClient->sync_getTransceiverInfo(
        transceiverInfo, getRequiredTransceivers(portEntries));

    return createModel(portEntries, transceiverInfo, queriedIfs);
  }

  std::vector<int32_t> getRequiredTransceivers(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries) {
    std::vector<int32_t> requiredTransceivers;
    for (const auto& port : portEntries) {
      if (auto transIdx = port.second.transceiverIdx()) {
        requiredTransceivers.push_back(transIdx->get_transceiverId());
      }
    }
    return requiredTransceivers;
  }

  Table::StyledCell colorStatusCell(std::string status) {
    Table::Style cellStyle = Table::Style::NONE;
    if (status == "up") {
      cellStyle = Table::Style::GOOD;
    }
    return Table::StyledCell(status, cellStyle);
  }

  Table::StyledCell colorTransCell(std::string transceiver) {
    Table::Style cellStyle = Table::Style::NONE;
    if (transceiver == "Not Present") {
      cellStyle = Table::Style::INFO;
    }
    return Table::StyledCell(transceiver, cellStyle);
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
      std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverEntries,
      const std::vector<std::string>& queriedIfs) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedIfs.begin(), queriedIfs.end());
    for (const auto& [portId, portInfo] : portEntries) {
      if (queriedIfs.size() == 0 || queriedSet.count(portInfo.get_name())) {
        cli::InterfaceStatus ifStatus;
        int32_t transceiverId =
            portInfo.get_transceiverIdx()->get_transceiverId();
        const auto& transceiver = transceiverEntries[transceiverId];
        const auto operState = portInfo.get_operState();

        ifStatus.name() = portInfo.get_name();
        ifStatus.description() = portInfo.get_description();
        ifStatus.status() =
            (operState == facebook::fboss::PortOperState::UP) ? "up" : "down";
        if (portInfo.get_vlans().size()) {
          ifStatus.vlan() = portInfo.get_vlans()[0];
        }
        ifStatus.speed() =
            std::to_string(portInfo.get_speedMbps() / 1000) + "G";
        if (transceiver.get_vendor()) {
          ifStatus.vendor() = transceiver.get_vendor()->get_name();
          ifStatus.mpn() = transceiver.get_vendor()->get_partNumber();
        } else {
          ifStatus.vendor() = "Not Present";
          ifStatus.mpn() = "Not Present";
        }

        model.interfaces()->push_back(ifStatus);
      }
    }
    std::sort(
        model.interfaces()->begin(),
        model.interfaces()->end(),
        [](cli::InterfaceStatus& a, cli::InterfaceStatus b) {
          return a.get_name() < b.get_name();
        });
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table outTable;

    outTable.setHeader({
        "Interface",
        "Description",
        "Status",
        "Vlan",
        "Speed",
        "Vendor",
        "Part Number",
    });

    for (const auto& portStatus : model.get_interfaces()) {
      outTable.addRow({
          portStatus.get_name(),
          portStatus.get_description(),
          colorStatusCell(portStatus.get_status()),
          (portStatus.vlan().has_value()
               ? std::to_string(*portStatus.get_vlan())
               : "--"),
          portStatus.get_speed(),
          colorTransCell(portStatus.get_vendor()),
          colorTransCell(portStatus.get_mpn()),
      });
    }

    out << outTable << std::endl;
  }
};

} // namespace facebook::fboss
