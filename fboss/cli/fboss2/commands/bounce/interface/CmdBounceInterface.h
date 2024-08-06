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

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

#include <string>
#include <unordered_set>

namespace facebook::fboss {

struct CmdBounceInterfaceTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::string;
};

class CmdBounceInterface
    : public CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits> {
 public:
  using RetType = CmdBounceInterfaceTraits::RetType;
  int SECONDS_TO_SLEEP = 5;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    /* Interface flap stats are stored as a FB303 counter.  Will call
       getRegexCounters so we can filter out just the interface counters and
       ignore the multitude of other counters we don't need.
    */
    std::string bounceResult = "";
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    client->sync_getAllPortInfo(portEntries);

    std::vector<int32_t> portsToBounce =
        utils::getPortIDList(queriedIfs, portEntries);

    for (const auto& port : portsToBounce) {
      fmt::print(fg(fmt::color::cyan), "Interface: ");
      std::cout << portEntries[port].get_name() << std::endl;

      std::string status = getPortStatus(port, portsToBounce, *client);

      /* Find the current status of the port, flip it to the opposite status
         sleep some seconds, and then flip it back.  Print update messages
         along the way.

         The sleep is necessary to allow the thrift calls to catch up with
         the speed of the CLI execution
      */
      if (status == "Up") {
        printActionText(0);
        changeInterfaceStatus(port, false, *client);
        status = getPortStatus(port, portsToBounce, *client);

        if (status == "Down") {
          printActionText(1);
          changeInterfaceStatus(port, true, *client);
          getPortStatus(port, portsToBounce, *client);
        }
      } else {
        printActionText(1);
        changeInterfaceStatus(port, true, *client);
        status = getPortStatus(port, portsToBounce, *client);

        if (status == "Up") {
          printActionText(0);
          changeInterfaceStatus(port, false, *client);
          getPortStatus(port, portsToBounce, *client);
        }
      }
    }

    return bounceResult;
  }

  void printActionText(int32_t actionCode) {
    switch (actionCode) {
      case 0:
        fmt::print(
            fg(fmt::color::light_slate_gray),
            "Shutting Down...Sleeping {} Seconds",
            std::to_string(SECONDS_TO_SLEEP));
        break;
      case 1:
        fmt::print(
            fg(fmt::color::light_slate_gray),
            "Bringing Up...Sleeping {} Seconds",
            std::to_string(SECONDS_TO_SLEEP));
        break;
    }
  }

  std::string getPortStatus(
      int32_t port,
      std::vector<int32_t> portsToBounce,
      FbossCtrlAsyncClient& client) {
    std::map<int32_t, facebook::fboss::PortStatus> portStates;
    std::string statusStr;
    client.sync_getPortStatus(portStates, portsToBounce);
    if (portStates.size() == 0) {
      throw std::runtime_error(
          "Unable to get port status for queried interfaces");
    }
    statusStr = (portStates[port].get_up()) ? "Up" : "Down";
    fmt::print(fg(fmt::color::cyan), "Port State: ");
    std::cout << statusStr << std::endl;
    return statusStr;
  }

  void changeInterfaceStatus(
      int32_t portId,
      bool enabled,
      FbossCtrlAsyncClient& client) {
    client.sync_setPortState(portId, enabled);
    sleep(SECONDS_TO_SLEEP);
  }

  void printOutput(const RetType& bounceResult) {
    std::cout << bounceResult << std::endl;
  }
};
} // namespace facebook::fboss
