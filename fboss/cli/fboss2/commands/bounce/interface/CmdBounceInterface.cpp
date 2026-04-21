/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/bounce/interface/CmdBounceInterface.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include <fmt/color.h>

namespace facebook::fboss {

CmdBounceInterface::RetType CmdBounceInterface::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedIfs) {
  /* Interface flap stats are stored as a FB303 counter.  Will call
     getRegexCounters so we can filter out just the interface counters and
     ignore the multitude of other counters we don't need.
  */
  std::string bounceResult;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  client->sync_getAllPortInfo(portEntries);

  std::vector<int32_t> portsToBounce =
      utils::getPortIDList(queriedIfs, portEntries);

  for (const auto& port : portsToBounce) {
    fmt::print(fg(fmt::color::cyan), "Interface: ");
    std::cout << portEntries[port].name().value() << std::endl;

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

void CmdBounceInterface::printActionText(int32_t actionCode) {
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

std::string CmdBounceInterface::getPortStatus(
    int32_t port,
    const std::vector<int32_t>& portsToBounce,
    apache::thrift::Client<FbossCtrl>& client) {
  std::map<int32_t, facebook::fboss::PortStatus> portStates;
  std::string statusStr;
  client.sync_getPortStatus(portStates, portsToBounce);
  if (portStates.size() == 0) {
    throw std::runtime_error(
        "Unable to get port status for queried interfaces");
  }
  statusStr = (folly::copy(portStates[port].up().value())) ? "Up" : "Down";
  fmt::print(fg(fmt::color::cyan), "Port State: ");
  std::cout << statusStr << std::endl;
  return statusStr;
}

void CmdBounceInterface::changeInterfaceStatus(
    int32_t portId,
    bool enabled,
    apache::thrift::Client<FbossCtrl>& client) {
  client.sync_setPortState(portId, enabled);
  // NOLINTNEXTLINE(facebook-hte-BadCall-sleep)
  sleep(SECONDS_TO_SLEEP);
}

void CmdBounceInterface::printOutput(const RetType& bounceResult) {
  std::cout << bounceResult << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdBounceInterface, CmdBounceInterfaceTraits>::getValidFilters();

} // namespace facebook::fboss
