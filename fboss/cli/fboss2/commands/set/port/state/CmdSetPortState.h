// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <boost/algorithm/string.hpp>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/set/port/CmdSetPort.h"

namespace {
inline std::map<std::string, int32_t> getQueriedPortIds(
    const std::map<int32_t, facebook::fboss::PortInfoThrift>& entries,
    const std::vector<std::string>& queriedPorts) {
  // deduplicate repetitive names while ensuring order
  std::map<std::string, int32_t> portPairs;
  std::unordered_map<std::string, int32_t> entryNames;

  for (const auto& entry : entries) {
    auto portInfo = entry.second;
    entryNames.emplace(portInfo.get_name(), portInfo.get_portId());
  }

  for (const auto& port : queriedPorts) {
    if (entryNames.count(port)) {
      portPairs.emplace(port, entryNames.at(port));
    } else {
      throw std::runtime_error(folly::to<std::string>(
          "Error: Invalid value for '[ports]...': ",
          port,
          " is not a valid Port"));
    }
  }
  return portPairs;
}

} // namespace

namespace facebook::fboss {

struct CmdSetPortStateTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PORT_STATE;
  using ParentCmd = CmdSetPort;
  using ObjectArgType = utils::PortState;
  using RetType = std::string;
};

class CmdSetPortState
    : public CmdHandler<CmdSetPortState, CmdSetPortStateTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedPorts,
      const utils::PortState& state) {
    std::string stateStr = (state.portState) ? "Enabling" : "Disabling";

    std::map<int32_t, facebook::fboss::PortInfoThrift> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    client->sync_getAllPortInfo(entries);

    std::map<std::string, int32_t> queriedPortIds =
        getQueriedPortIds(entries, queriedPorts.data());

    std::stringstream ss;
    for (auto const& [portName, portId] : queriedPortIds) {
      client->sync_setPortState(portId, state.portState);
      ss << stateStr << " port " << portName << std::endl;
    }

    return ss.str();
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << model;
  }
};

} // namespace facebook::fboss
