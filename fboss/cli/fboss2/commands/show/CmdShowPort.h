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
#include "fboss/cli/fboss2/utils/Table.h"

#include <unistd.h>

namespace facebook::fboss {

using utils::Table;

struct CmdShowPortTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::map<int32_t, facebook::fboss::PortInfoThrift>;
};

class CmdShowPort : public CmdHandler<CmdShowPort, CmdShowPortTraits> {
 public:
  using ObjectArgType = CmdShowPortTraits::ObjectArgType;
  using RetType = CmdShowPortTraits::RetType;

  RetType queryClient(
      const folly::IPAddress& hostIp,
      const ObjectArgType& queriedPorts) {
    RetType portEntries;

    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());

    client->sync_getAllPortInfo(portEntries);

    if (queriedPorts.size() == 0) {
      return portEntries;
    }

    RetType retVal;
    for (auto const& [portId, portInfo] : portEntries) {
      for (auto const& queriedPort : queriedPorts) {
        if (portInfo.get_name() == queriedPort) {
          retVal.insert(std::make_pair(portId, portInfo));
        }
      }
    }

    return retVal;
  }

  void printOutput(const RetType& portId2PortInfoThrift) {
    Table table;
    table.setHeader(
        {"ID", "Name", "AdminState", "LinkState", "Speed", "ProfileID"});

    for (auto const& [portId, portInfo] : portId2PortInfoThrift) {
      table.addRow({
          folly::to<std::string>(portId),
          portInfo.get_name(),
          getAdminStateStr(portInfo.get_adminState()),
          getStyledOperState(portInfo.get_operState()),
          getSpeedGbps(portInfo.get_speedMbps()),
          portInfo.get_profileID(),
      });
    }

    std::cout << table << std::endl;
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

  Table::StyledCell getStyledOperState(PortOperState operState) {
    switch (operState) {
      case PortOperState::DOWN:
        return Table::StyledCell("Down", Table::Style::WARN);
      case PortOperState::UP:
        return Table::StyledCell("Up", Table::Style::GOOD);
    }

    throw std::runtime_error(
        "Unsupported LinkState: " +
        std::to_string(static_cast<int>(operState)));
  }

  std::string getSpeedGbps(int64_t speedMbps) {
    return std::to_string(speedMbps / 1000) + "G";
  }
};

} // namespace facebook::fboss
