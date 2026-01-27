/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/vlan/static_mac/add/CmdConfigVlanStaticMacAdd.h"

#include <fmt/format.h>
#include <algorithm>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace facebook::fboss {

CmdConfigVlanStaticMacAddTraits::RetType CmdConfigVlanStaticMacAdd::queryClient(
    const HostInfo& hostInfo,
    const VlanId& vlanIdArg,
    const ObjectArgType& macAndPort) {
  auto& config = ConfigSession::getInstance().getAgentConfig();
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

  // Get port logical ID from port name
  const auto& portMap = ConfigSession::getInstance().getPortMap();
  auto portLogicalId = portMap.getPortLogicalId(macAndPort.getPortName());
  if (!portLogicalId.has_value()) {
    throw std::invalid_argument(
        fmt::format(
            "Port '{}' not found in configuration", macAndPort.getPortName()));
  }

  const std::string& macAddress = macAndPort.getMacAddress();

  // Check if entry already exists - if so, return success (idempotent)
  if (swConfig.staticMacAddrs().has_value()) {
    for (const auto& entry : *swConfig.staticMacAddrs()) {
      if (*entry.vlanID() == vlanId && *entry.macAddress() == macAddress) {
        return fmt::format(
            "Static MAC entry for MAC {} on VLAN {} already exists",
            macAddress,
            vlanId);
      }
    }
  }

  // Create and add the new static MAC entry
  cfg::StaticMacEntry newEntry;
  newEntry.vlanID() = vlanId;
  newEntry.macAddress() = macAddress;
  newEntry.egressLogicalPortID() = static_cast<int32_t>(*portLogicalId);

  if (!swConfig.staticMacAddrs().has_value()) {
    swConfig.staticMacAddrs() = std::vector<cfg::StaticMacEntry>();
  }
  swConfig.staticMacAddrs()->push_back(newEntry);

  // Save the updated config
  ConfigSession::getInstance().saveConfig();

  return fmt::format(
      "Successfully added static MAC entry: MAC {} -> VLAN {}, Port {} (ID {})",
      macAddress,
      vlanId,
      macAndPort.getPortName(),
      static_cast<int32_t>(*portLogicalId));
}

void CmdConfigVlanStaticMacAdd::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
