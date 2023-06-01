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
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"

namespace facebook::fboss {

struct CmdShowPortQueueTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowPort;
  // This command will inherit port list from its parent (ShowPort)
  // NoneArgType indicates that there is no args input
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::map<int32_t, facebook::fboss::PortInfoThrift>;
};

class CmdShowPortQueue
    : public CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits> {
 public:
  using ObjectArgType = CmdShowPortQueueTraits::ObjectArgType;
  using RetType = CmdShowPortQueueTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedPorts) {
    RetType portEntries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getAllPortInfo(portEntries);

    if (queriedPorts.size() == 0) {
      return portEntries;
    }

    RetType retVal;
    for (auto const& [portId, portInfo] : portEntries) {
      for (auto const& queriedPort : queriedPorts.data()) {
        if (portInfo.get_name() == queriedPort) {
          retVal.insert(std::make_pair(portId, portInfo));
        }
      }
    }

    return retVal;
  }

  void printOutput(const RetType& portId2PortInfoThrift) {
    constexpr auto fmtString = "{:<7}{:<20}{:<25}{:<10}{:<15}{:<15}\n";

    for (auto const& [portId, portInfo] : portId2PortInfoThrift) {
      std::ignore = portId;

      std::cout << portInfo.get_name() << "\n";
      std::cout << std::string(10, '=') << std::endl;

      std::cout << fmt::format(
          fmtString,
          "ID",
          "Name",
          "Mode",
          "Weight",
          "ReservedBytes",
          "ScalingFactor");
      std::cout << std::string(90, '-') << std::endl;

      for (auto const& queue : portInfo.get_portQueues()) {
        std::cout << fmt::format(
            fmtString,
            queue.get_id(),
            queue.get_name(),
            queue.get_mode(),
            queue.weight() ? std::to_string(*queue.weight()) : "",
            queue.reservedBytes() ? std::to_string(*queue.reservedBytes()) : "",
            queue.scalingFactor() ? *queue.scalingFactor() : "");
      }

      std::cout << "\n";
    }
  }
};

} // namespace facebook::fboss
