// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/start/port/cable_length_measurement/CmdStartPortCableLengthMeasurement.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <stdexcept>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

namespace {

std::vector<int32_t> getPortIdsFromSwitchState(
    const state::SwitchState& switchState,
    const std::vector<std::string>& queriedPorts) {
  std::map<std::string, int32_t> portNameToId;
  for (const auto& [_, portMap] : *switchState.portMaps()) {
    for (const auto& [__, port] : portMap) {
      portNameToId[*port.portName()] = *port.portId();
    }
  }

  std::vector<int32_t> queriedPortIds;
  queriedPortIds.reserve(queriedPorts.size());
  for (const auto& port : queriedPorts) {
    auto portId = portNameToId.find(port);
    if (portId == portNameToId.end()) {
      throw std::runtime_error(
          fmt::format("{} is not a valid port on this device", port));
    }
    queriedPortIds.push_back(portId->second);
  }
  return queriedPortIds;
}

} // namespace

CmdStartPortCableLengthMeasurement::RetType
CmdStartPortCableLengthMeasurement::queryClient(
    const HostInfo& hostInfo,
    const utils::PortList& ports) {
  if (ports.data().empty()) {
    throw std::runtime_error("At least one port is required");
  }

  auto switchIndicesForPorts =
      utils::getSwitchIndicesForInterfaces(hostInfo, ports.data());
  if (switchIndicesForPorts.empty()) {
    throw std::runtime_error("No matching ports found");
  }

  std::vector<std::string> triggeredPorts;
  for (const auto& [switchIndex, switchPorts] : switchIndicesForPorts) {
    auto hwAgentClient =
        utils::createClient<apache::thrift::Client<FbossHwCtrl>>(
            hostInfo, switchIndex);

    state::SwitchState switchState;
    hwAgentClient->sync_getProgrammedState(switchState);
    auto portIds = getPortIdsFromSwitchState(switchState, switchPorts);

    triggeredPorts.insert(
        triggeredPorts.end(), switchPorts.begin(), switchPorts.end());
    hwAgentClient->sync_triggerCableLengthMeasurement(portIds);
  }

  return fmt::format(
      "Triggered cable length measurement on port(s): {}",
      folly::join(", ", triggeredPorts));
}

void CmdStartPortCableLengthMeasurement::printOutput(
    const RetType& model,
    std::ostream& out) {
  out << model << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdStartPortCableLengthMeasurement,
    CmdStartPortCableLengthMeasurementTraits>::run();

} // namespace facebook::fboss
