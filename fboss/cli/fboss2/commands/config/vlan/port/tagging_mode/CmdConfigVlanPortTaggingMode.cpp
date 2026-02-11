/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/vlan/port/tagging_mode/CmdConfigVlanPortTaggingMode.h"

#include <fmt/format.h>
#include <algorithm>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace facebook::fboss {

CmdConfigVlanPortTaggingModeTraits::RetType
CmdConfigVlanPortTaggingMode::queryClient(
    const HostInfo& hostInfo,
    const VlanId& vlanIdArg,
    const utils::PortList& portList,
    const ObjectArgType& taggingMode) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();
  int32_t vlanId = vlanIdArg.getVlanId();

  // Check if VLAN exists in configuration
  auto vitr = std::find_if(
      swConfig.vlans()->cbegin(),
      swConfig.vlans()->cend(),
      [vlanId](const auto& vlan) { return *vlan.id() == vlanId; });

  if (vitr == swConfig.vlans()->cend()) {
    throw std::invalid_argument(
        fmt::format("VLAN {} does not exist in configuration", vlanId));
  }

  const auto& ports = portList.data();
  if (ports.empty()) {
    throw std::invalid_argument("No port name provided");
  }

  // Get port logical IDs from port names
  const auto& portMap = session.getPortMap();
  std::vector<std::pair<std::string, int32_t>> portNamesAndIds;
  for (const auto& portName : ports) {
    auto portLogicalId = portMap.getPortLogicalId(portName);
    if (!portLogicalId.has_value()) {
      throw std::invalid_argument(
          fmt::format("Port '{}' not found in configuration", portName));
    }
    portNamesAndIds.emplace_back(
        portName, static_cast<int32_t>(*portLogicalId));
  }

  bool emitTags = taggingMode.getEmitTags();
  std::vector<std::string> updatedPorts;

  // Update VlanPort entries
  for (const auto& [portName, portLogicalId] : portNamesAndIds) {
    bool found = false;
    for (auto& vlanPort : *swConfig.vlanPorts()) {
      if (*vlanPort.vlanID() == vlanId &&
          *vlanPort.logicalPort() == portLogicalId) {
        vlanPort.emitTags() = emitTags;
        found = true;
        updatedPorts.push_back(portName);
        break;
      }
    }

    if (!found) {
      throw std::invalid_argument(
          fmt::format(
              "Port '{}' (ID {}) is not a member of VLAN {}",
              portName,
              portLogicalId,
              vlanId));
    }
  }

  // Save the updated config - tagging mode changes are hitless (no agent
  // restart)
  session.saveConfig();

  std::string modeStr = emitTags ? "tagged" : "untagged";
  if (updatedPorts.size() == 1) {
    return fmt::format(
        "Successfully set port {} tagging mode to {} on VLAN {}",
        updatedPorts[0],
        modeStr,
        vlanId);
  } else {
    return fmt::format(
        "Successfully set {} ports tagging mode to {} on VLAN {}",
        updatedPorts.size(),
        modeStr,
        vlanId);
  }
}

void CmdConfigVlanPortTaggingMode::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
