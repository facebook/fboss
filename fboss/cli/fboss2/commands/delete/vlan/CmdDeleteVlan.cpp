/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/vlan/CmdDeleteVlan.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/commands/config/vlan/VlanManager.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {

// Refuses (throws FbossError) rather than silently orphaning references or
// changing a sibling object's meaning when the VLAN is still in use. Each
// referrer must be cleared first:
//   - it is the global default VLAN (SwitchConfig.defaultVlan)
//       -> point the default elsewhere: config vlan default <other-id>
//   - a port lists it as its untagged ingress VLAN (Port.ingressVlan); the
//     only fallback value is 0, which means routed port, so clearing it here
//     would change the port's L2 mode
//       -> move the port: config interface <port> switchport access vlan
//          <other-id>
// Switchport membership (VlanPort) and the VLAN's interface are not referrers
// in this sense: VlanManager::deleteVlan cascades them away.
// Also throws FbossError if no VLAN with the given ID exists.
void checkDeletable(const cfg::SwitchConfig& swConfig, const VlanID& vlanId) {
  const auto id = static_cast<int32_t>(vlanId);

  if (VlanManager::findVlan(swConfig, vlanId) == nullptr) {
    throw FbossError("VLAN ", static_cast<uint16_t>(vlanId), " does not exist");
  }

  if (*swConfig.defaultVlan() == id) {
    throw FbossError(
        "Cannot delete VLAN ",
        static_cast<uint16_t>(vlanId),
        ": it is the global default VLAN");
  }

  std::vector<std::string> ingressPorts;
  for (const auto& port : *swConfig.ports()) {
    if (*port.ingressVlan() == id) {
      ingressPorts.push_back(
          port.name().has_value() ? *port.name()
                                  : std::to_string(*port.logicalID()));
    }
  }
  if (!ingressPorts.empty()) {
    throw FbossError(
        "Cannot delete VLAN ",
        static_cast<uint16_t>(vlanId),
        ": it is the ingress VLAN for port(s): ",
        folly::join(", ", ingressPorts));
  }
}

} // namespace

CmdDeleteVlanTraits::RetType CmdDeleteVlan::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& vlanIdArg) {
  auto& session = ConfigSession::getInstance();
  auto& swConfig = *session.getAgentConfig().sw();
  VlanID vlanId(vlanIdArg.getVlanId());

  checkDeletable(swConfig, vlanId);
  VlanManager::deleteVlan(swConfig, vlanId);

  // VLAN membership/creation is applied hitlessly, matching the config vlan
  // subcommands (all save with the default HITLESS action level).
  session.saveConfig();

  return fmt::format("Deleted VLAN {}", static_cast<uint16_t>(vlanId));
}

void CmdDeleteVlan::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdDeleteVlan, CmdDeleteVlanTraits>::run();

} // namespace facebook::fboss
