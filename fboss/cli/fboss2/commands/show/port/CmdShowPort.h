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
    std::vector<int32_t> requiredTransceiverEntries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    client->sync_getAllPortInfo(portEntries);

    try {
      auto qsfpService =
          utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(
              hostInfo);

      qsfpService->sync_getTransceiverInfo(
          transceiverEntries, requiredTransceiverEntries);
    } catch (apache::thrift::transport::TTransportException& e) {
      std::cerr << "Cannot connect to qsfp_service\n";
    }

    return createModel(portEntries, transceiverEntries, queriedPorts.data());
  }

  std::unordered_map<std::string, std::vector<std::string>>
  getAcceptedFilterValues() {
    return {
        {"linkState", {"Up", "Down"}}, {"adminState", {"Enabled", "Disabled"}}};
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"ID",
         "Name",
         "AdminState",
         "LinkState",
         "Transceiver",
         "TcvrID",
         "Speed",
         "ProfileID",
         "HwLogicalPortId"});

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
           portInfo.get_tcvrPresent(),
           folly::to<std::string>(portInfo.get_tcvrID()),
           portInfo.get_speed(),
           portInfo.get_profileId(),
           hwLogicalPortId});
    }

    out << table << std::endl;
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

  std::string getTransceiverStr(
      std::map<int32_t, facebook::fboss::TransceiverInfo>& transceiverEntries,
      int32_t transceiverId) {
    if (transceiverEntries.count(transceiverId) == 0) {
      return "";
    }
    auto isPresent = transceiverEntries[transceiverId].get_present();
    if (isPresent)
      return "Present";
    return "Absent";
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
      std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverEntries,
      const ObjectArgType& queriedPorts) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedPorts.begin(), queriedPorts.end());

    for (const auto& entry : portEntries) {
      auto portInfo = entry.second;
      auto portName = portInfo.get_name();

      auto operState = getOperStateStr(portInfo.get_operState());

      if (queriedPorts.size() == 0 || queriedSet.count(portName)) {
        cli::PortEntry portDetails;
        portDetails.id() = portInfo.get_portId();
        portDetails.name() = portInfo.get_name();
        portDetails.adminState() = getAdminStateStr(portInfo.get_adminState());
        portDetails.linkState() = operState;
        portDetails.speed() = utils::getSpeedGbps(portInfo.get_speedMbps());
        portDetails.profileId() = portInfo.get_profileID();
        if (auto hwLogicalPortId = portInfo.hwLogicalPortId()) {
          portDetails.hwLogicalPortId() = *hwLogicalPortId;
        }
        if (auto tcvrId = portInfo.transceiverIdx()) {
          const auto transceiverId = tcvrId->get_transceiverId();
          portDetails.tcvrID() = transceiverId;
          portDetails.tcvrPresent() =
              getTransceiverStr(transceiverEntries, transceiverId);
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
