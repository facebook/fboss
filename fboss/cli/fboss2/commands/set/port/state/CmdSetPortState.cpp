// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/port/state/CmdSetPortState.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/SafetyPromptUtils.h"

#include <fmt/format.h>
#include <folly/String.h>

namespace facebook::fboss {

namespace {
constexpr auto kSetPortStateWarning = R"WARN(
================================================================================
WARNING: `fboss2 set port state` directly toggles port admin state on the
switch. This can cause IMMEDIATE TRAFFIC IMPACT.

Running this on a production device WITHOUT the device/circuit being drained
is a violation of the Safer Human Touch (SHT) Policy. Every invocation of
`fboss2 set port state` is logged and reviewed.

Prefer:  cableguy_cli bounce_interface --circuit "<a:intf|z:intf>" --task <T#>
         (validates circuit/device drain state before bouncing)
================================================================================
)WARN";
} // namespace

std::map<std::string, int32_t> getQueriedPortIds(
    const std::map<int32_t, facebook::fboss::PortInfoThrift>& entries,
    const std::vector<std::string>& queriedPorts,
    const std::vector<facebook::fboss::AggregatePortThrift>& aggregatePorts) {
  // deduplicate repetitive names while ensuring order
  std::map<std::string, int32_t> portPairs;
  std::unordered_map<std::string, int32_t> entryNames;

  for (const auto& entry : entries) {
    auto portInfo = entry.second;
    entryNames.emplace(
        portInfo.name().value(), folly::copy(portInfo.portId().value()));
  }

  // Build a map of aggregate port names to their member port IDs
  std::unordered_map<std::string, std::vector<int32_t>> aggregatePortMembers;
  for (const auto& aggPort : aggregatePorts) {
    std::vector<int32_t> memberIds;
    for (const auto& member : *aggPort.memberPorts()) {
      memberIds.push_back(folly::copy(member.memberPortID().value()));
    }
    aggregatePortMembers.emplace(aggPort.name().value(), std::move(memberIds));
  }

  for (const auto& port : queriedPorts) {
    if (entryNames.count(port)) {
      // Direct port match
      portPairs.emplace(port, entryNames.at(port));
    } else if (aggregatePortMembers.count(port)) {
      // Aggregate port match - expand to all member ports
      for (const auto& memberId : aggregatePortMembers.at(port)) {
        // Find the member port name from entries
        auto it = entries.find(memberId);
        if (it != entries.end()) {
          const auto& memberName = it->second.name().value();
          portPairs.emplace(memberName, memberId);
        }
      }
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
  const auto target = fmt::format(
      "{} port(s) [{}] on {}",
      state.portState ? "ENABLE" : "DISABLE",
      folly::join(", ", queriedPorts.data()),
      hostInfo.getName());
  utils::requireConfirmation(
      kSetPortStateCommandName,
      kSetPortStateYesFlag,
      kSetPortStateWarning,
      target);

  std::string stateStr = (state.portState) ? "Enabling" : "Disabling";

  std::map<int32_t, facebook::fboss::PortInfoThrift> entries;
  std::vector<facebook::fboss::AggregatePortThrift> aggregatePorts;
  auto client =
      utils::createClient<apache::thrift::Client<facebook::fboss::FbossCtrl>>(
          hostInfo);
  client->sync_getAllPortInfo(entries);
  client->sync_getAggregatePortTable(aggregatePorts);

  std::map<std::string, int32_t> queriedPortIds =
      getQueriedPortIds(entries, queriedPorts.data(), aggregatePorts);

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
