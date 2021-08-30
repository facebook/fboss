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

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <unistd.h>

namespace facebook::fboss {

using utils::Table;

struct CmdShowPortTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowPortModel;
};

class CmdShowPort : public CmdHandler<CmdShowPort, CmdShowPortTraits> {
 public:
  using ObjectArgType = CmdShowPortTraits::ObjectArgType;
  using RetType = CmdShowPortTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts) {
    std::map<int32_t, facebook::fboss::PortInfoThrift> entries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getAllPortInfo(entries);

    return createModel(entries, queriedPorts);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"ID",
         "Name",
         "AdminState",
         "LinkState",
         "TcvrID",
         "Speed",
         "ProfileID"});

    for (auto const& portInfo : model.get_portEntries()) {
      table.addRow({
          folly::to<std::string>(portInfo.get_id()),
          portInfo.get_name(),
          portInfo.get_adminState(),
          getStyledOperState(portInfo.get_operState()),
          folly::to<std::string>(portInfo.get_tcvrID()),
          portInfo.get_speed(),
          portInfo.get_profileId(),
      });
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

  Table::StyledCell getStyledOperState(std::string operState) {
    if (operState == "Down") {
      return Table::StyledCell("Down", Table::Style::ERROR);
    } else {
      return Table::StyledCell("Up", Table::Style::GOOD);
    }

    throw std::runtime_error("Unsupported LinkState: " + operState);
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

  std::string getSpeedGbps(int64_t speedMbps) {
    return std::to_string(speedMbps / 1000) + "G";
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
      const ObjectArgType& queriedPorts) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedPorts.begin(), queriedPorts.end());

    for (const auto& entry : portEntries) {
      auto portInfo = entry.second;
      auto portName = portInfo.get_name();

      if (queriedPorts.size() == 0 || queriedSet.count(portName)) {
        cli::PortEntry portDetails;
        portDetails.id_ref() = portInfo.get_portId();
        portDetails.name_ref() = portInfo.get_name();
        portDetails.adminState_ref() =
            getAdminStateStr(portInfo.get_adminState());
        portDetails.operState_ref() = getOperStateStr(portInfo.get_operState());
        portDetails.speed_ref() = getSpeedGbps(portInfo.get_speedMbps());
        portDetails.profileId_ref() = portInfo.get_profileID();
        if (auto tcvrId = portInfo.transceiverIdx_ref()) {
          portDetails.tcvrID_ref() = tcvrId->get_transceiverId();
        }

        model.portEntries_ref()->push_back(portDetails);
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
