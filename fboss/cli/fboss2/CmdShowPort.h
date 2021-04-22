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

namespace facebook::fboss {

struct CmdShowPortTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::map<int32_t, facebook::fboss::PortInfoThrift>;
};

class CmdShowPort : public CmdHandler<CmdShowPort, CmdShowPortTraits> {
 public:
  using RetType = CmdShowPortTraits::RetType;

  RetType queryClient(const folly::IPAddress& hostIp) {
    RetType retVal;
    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());

    client->sync_getAllPortInfo(retVal);
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

    for (auto const&[portId, portInfo] : portId2PortInfoThrift) {
      std::cout << fmt::format(fmtString,
                               portInfo.get_portId(),
                               portInfo.get_name(),
                               getAdminStateStr(portInfo.get_adminState()),
                               getOperStateStr(portInfo.get_operState()),
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
};

} // namespace facebook::fboss
