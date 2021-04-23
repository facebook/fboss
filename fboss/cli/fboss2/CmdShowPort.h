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

#include <unistd.h>

namespace facebook::fboss {

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
    for (auto const&[portId, portInfo] : portEntries) {
      for (auto const& queriedPort : queriedPorts) {
        if (portInfo.get_name() == queriedPort) {
          retVal.insert(std::make_pair(portId, portInfo));
        }
      }
    }

    return retVal;
  }

  void printOutput(const RetType& portId2PortInfoThrift) {
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
    for (auto const&[portId, portInfo] : portId2PortInfoThrift) {
      std::ignore = portId;
      auto [operState, operColor] = getOperStateStr(portInfo.get_operState());

      fmt::print(
          "{:<7}{:<15}{:<15}",
          portInfo.get_portId(),
          portInfo.get_name(),
          getAdminStateStr(portInfo.get_adminState()));

      if ((CmdGlobalOptions::getInstance()->getColor() == "yes") &&
          isatty(fileno(stdout))) {
        fmt::print(fmt::emphasis::bold | fg(operColor), "{0:<15}", operState);
      } else {
        fmt::print("{0:<15}", operState);
      }

      fmt::print(
          "{:<10}{:<20}\n",
          getSpeedGbps(portInfo.get_speedMbps()),
          portInfo.get_profileID());
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

  std::pair<std::string, fmt::color> getOperStateStr(
      PortOperState operState) {
    switch (operState) {
      case PortOperState::DOWN:
        return std::make_pair("Down", fmt::color::red);
      case PortOperState::UP:
        return std::make_pair("Up", fmt::color::green);
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
