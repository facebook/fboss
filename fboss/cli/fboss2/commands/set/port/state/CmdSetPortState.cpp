// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/port/state/CmdSetPortState.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

std::map<std::string, int32_t> getQueriedPortIds(
    const std::map<int32_t, facebook::fboss::PortInfoThrift>& entries,
    const std::vector<std::string>& queriedPorts) {
  // deduplicate repetitive names while ensuring order
  std::map<std::string, int32_t> portPairs;
  std::unordered_map<std::string, int32_t> entryNames;

  for (const auto& entry : entries) {
    auto portInfo = entry.second;
    entryNames.emplace(
        portInfo.name().value(), folly::copy(portInfo.portId().value()));
  }

  for (const auto& port : queriedPorts) {
    if (entryNames.count(port)) {
      portPairs.emplace(port, entryNames.at(port));
    } else {
      throw std::runtime_error(
          folly::to<std::string>(
              "Error: Invalid value for '[ports]...': ",
              port,
              " is not a valid Port"));
    }
  }
  return portPairs;
}

CmdSetPortState::RetType CmdSetPortState::queryClient(
    const HostInfo& hostInfo,
    const utils::PortList& queriedPorts,
    const utils::PortState& state) {
  std::string stateStr = (state.portState) ? "Enabling" : "Disabling";

  std::map<int32_t, facebook::fboss::PortInfoThrift> entries;
  auto client =
      utils::createClient<apache::thrift::Client<facebook::fboss::FbossCtrl>>(
          hostInfo);
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

void CmdSetPortState::printOutput(const RetType& model, std::ostream& out) {
  out << model;
}

// Explicit template instantiation
template void CmdHandler<CmdSetPortState, CmdSetPortStateTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdSetPortState, CmdSetPortStateTraits>::getValidFilters();

} // namespace facebook::fboss
