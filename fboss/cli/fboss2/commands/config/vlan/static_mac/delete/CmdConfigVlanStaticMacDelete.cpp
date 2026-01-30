/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/vlan/static_mac/delete/CmdConfigVlanStaticMacDelete.h"

#include <fmt/format.h>
#include <algorithm>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigVlanStaticMacDeleteTraits::RetType
CmdConfigVlanStaticMacDelete::queryClient(
    const HostInfo& hostInfo,
    const VlanId& vlanIdArg,
    const ObjectArgType& macAddressArg) {
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

  const std::string& macAddress = macAddressArg.getMacAddress();

  // Check if staticMacAddrs exists and find the entry to delete
  // If no entries exist or entry not found, return success (idempotent)
  if (!swConfig.staticMacAddrs().has_value() ||
      swConfig.staticMacAddrs()->empty()) {
    return fmt::format(
        "Static MAC entry for MAC {} on VLAN {} does not exist (no entries configured)",
        macAddress,
        vlanId);
  }

  auto& staticMacs = *swConfig.staticMacAddrs();
  auto it = std::find_if(
      staticMacs.begin(),
      staticMacs.end(),
      [vlanId, &macAddress](const auto& entry) {
        return *entry.vlanID() == vlanId && *entry.macAddress() == macAddress;
      });

  if (it == staticMacs.end()) {
    return fmt::format(
        "Static MAC entry for MAC {} on VLAN {} does not exist",
        macAddress,
        vlanId);
  }

  // Remove the entry
  staticMacs.erase(it);

  // Save the updated config
  ConfigSession::getInstance().saveConfig();

  return fmt::format(
      "Successfully deleted static MAC entry: MAC {} from VLAN {}",
      macAddress,
      vlanId);
}

void CmdConfigVlanStaticMacDelete::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
