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

#include <unistd.h>

namespace facebook::fboss {

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
      const folly::IPAddress& hostIp,
      const ObjectArgType& queriedPorts) {
    std::map<int32_t, facebook::fboss::PortInfoThrift> entries;

    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());

    client->sync_getAllPortInfo(entries);

    return createModel(entries, queriedPorts);
  }

  void printOutput(const RetType& model) {
    std::string fmtString = "{:<7}{:<15}{:<15}{:<15}{:<10}{:<20}\n";

    std::cout << fmt::format(
        fmtString,
        "ID",
        "Name",
        "AdminState",
        "LinkState",
        "Speed",
        "ProfileID");
    std::cout << std::string(90, '-') << std::endl;

    // TODO Factor out colored printing to a separate class that could be used
    // by multiple subcommands
    // explore if fmt library offers simpler way to nest coloring with
    // indented formatting.
    for (auto const& portInfo : model.get_portEntries()) {
      auto [operState, operColor] = getOperStateColor(portInfo.get_operState());

      fmt::print(
          "{:<7}{:<15}{:<15}",
          portInfo.get_id(),
          portInfo.get_name(),
          portInfo.get_adminState());

      if ((CmdGlobalOptions::getInstance()->getColor() == "yes") &&
          isatty(fileno(stdout))) {
        fmt::print(fmt::emphasis::bold | fg(operColor), "{0:<15}", operState);
      } else {
        fmt::print("{0:<15}", operState);
      }

      fmt::print(
          "{:<10}{:<20}\n",
          portInfo.get_speed(),
          portInfo.get_profileId());
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


  std::string getOperStateStr(
      PortOperState operState) {
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

  std::pair<std::string, fmt::color> getOperStateColor(
      std::string operState) {
    if (operState == "Down") {
        return std::make_pair("Down", fmt::color::red);
    } else {
        return std::make_pair("Up", fmt::color::green);
    }
  }

  std::string getSpeedGbps(int64_t speedMbps) {
    return std::to_string(speedMbps / 1000) + "G";
  }

  RetType createModel(
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
    const ObjectArgType& queriedPorts
  ) {
    RetType model;
    std::unordered_set<std::string> queriedSet(queriedPorts.begin(), queriedPorts.end());

    for (const auto& entry : portEntries) {
      auto portInfo = entry.second;
      auto portName = portInfo.get_name();

      if (queriedPorts.size() == 0 || queriedSet.count(portName)) {
        cli::PortEntry portDetails;
        portDetails.id_ref() = portInfo.get_portId();
        portDetails.name_ref() = portInfo.get_name();
        portDetails.adminState_ref() = getAdminStateStr(portInfo.get_adminState());
        portDetails.operState_ref() = getOperStateStr(portInfo.get_operState());
        portDetails.speed_ref() = getSpeedGbps(portInfo.get_speedMbps());
        portDetails.profileId_ref() = portInfo.get_profileID();

        model.portEntries_ref()->push_back(portDetails);
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
