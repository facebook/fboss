// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/clear/interface/counters/phy/CmdClearInterfaceCountersPhy.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <algorithm>

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

std::string CmdClearInterfaceCountersPhy::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedIfs) {
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  std::vector<int32_t> portsToClear;
  /*
    if the user has provided specific interfaces to clear then find the
    corresponding port names, otherwise empty vector means clear all counters
  */
  client->sync_getAllPortInfo(portEntries);
  if (queriedIfs.size()) {
    for (auto queriedIf : queriedIfs) {
      auto it = std::find_if(
          portEntries.begin(),
          portEntries.end(),
          [&queriedIf](const auto& port) {
            return port.second.name().value() == queriedIf;
          });
      if (it != portEntries.end()) {
        portsToClear.push_back(it->first);
      } else {
        throw std::runtime_error(
            fmt::format(
                "{} is not a valid interface name on this device", queriedIf));
      }
    }
  } else {
    for (auto& port : portEntries) {
      portsToClear.push_back(port.first);
    }
  }
  clearCountersPhy(hostInfo, portsToClear);

  return "Counters cleared.";
}

void CmdClearInterfaceCountersPhy::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

void CmdClearInterfaceCountersPhy::clearCountersPhy(
    const HostInfo& hostInfo,
    std::vector<int32_t> ports) {
  if (!ports.size()) {
    return;
  }

  auto hwAgentClearInterfaceCountersFunc =
      [&ports](apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
        client.sync_clearInterfacePhyCounters(ports);
      };
  utils::runOnAllHwAgents(hostInfo, hwAgentClearInterfaceCountersFunc);
}

// Explicit template instantiation
template void CmdHandler<
    CmdClearInterfaceCountersPhy,
    CmdClearInterfaceCountersPhyTraits>::run();

} // namespace facebook::fboss
