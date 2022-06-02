// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <boost/algorithm/string.hpp>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/set/port/CmdSetPort.h"

namespace {
inline bool getPortState(const std::vector<std::string>& args) {
  auto state = folly::gen::from(args) |
      folly::gen::mapped([](const std::string& s) {
                 return boost::to_upper_copy(s);
               }) |
      folly::gen::as<std::vector>();
  bool enable;
  if (state[0] == "ENABLE") {
    enable = true;
  } else if (state[0] == "DISABLE") {
    enable = false;
  } else {
    throw std::runtime_error(folly::to<std::string>(
        "Unexpected state '", args[0], "', expecting 'enable|disable'"));
  }
  return enable;
}

inline std::unordered_map<std::string, int32_t> getQueriedPortIds(
    const std::map<int32_t, facebook::fboss::PortInfoThrift>& entries,
    const std::vector<std::string>& queriedPorts) {
  // deduplicate repetitive names
  std::unordered_map<std::string, int32_t> portPairs;
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
  using ParentCmd = CmdSetPort;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PORT_STATE;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::string;
};

class CmdSetPortState
    : public CmdHandler<CmdSetPortState, CmdSetPortStateTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedPorts,
      const std::vector<std::string>& state) {
    if (state.empty()) {
      throw std::runtime_error(
          "Incomplete command, expecting 'set port <port_list> state <enable|disable>'");
    }
    if (state.size() != 1) {
      throw std::runtime_error(folly::to<std::string>(
          "Unexpected state '",
          folly::join<std::string, std::vector<std::string>>(" ", state),
          "', expecting 'enable|disable'"));
    }
    bool enable = getPortState(state);
    std::string stateStr = (enable) ? "Enabling" : "Disabling";

    std::map<int32_t, facebook::fboss::PortInfoThrift> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    client->sync_getAllPortInfo(entries);

    std::unordered_map<std::string, int32_t> queriedPortIds =
        getQueriedPortIds(entries, queriedPorts);

    std::stringstream ss;
    for (auto const& [portName, portId] : queriedPortIds) {
      client->sync_setPortState(portId, enable);
      ss << stateStr << " port " << portName << std::endl;
    }

    return ss.str();
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << model;
  }
};

} // namespace facebook::fboss
