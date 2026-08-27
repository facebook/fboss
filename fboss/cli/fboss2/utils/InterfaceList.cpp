/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/utils/InterfaceList.h"
#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace facebook::fboss::utils {

namespace {

constexpr std::string_view kVlanNamePrefix = "vlan";

// Parses a "vlan<digits>" SVI name (lowercase prefix, e.g. "vlan2001") into
// its VLAN ID. Returns nullopt for any other shape.
std::optional<VlanID> parseVlanName(const std::string& name) {
  if (name.size() <= kVlanNamePrefix.size() ||
      name.compare(0, kVlanNamePrefix.size(), kVlanNamePrefix) != 0) {
    return std::nullopt;
  }
  auto id = folly::tryTo<int32_t>(name.substr(kVlanNamePrefix.size()));
  if (!id.hasValue()) {
    return std::nullopt;
  }
  // VlanID is uint16_t; an unbounded int32 would wrap (vlan65537 -> VLAN 1)
  // and mutate the wrong SVI. Reject anything outside the 802.1Q range.
  if (*id < 1 || *id > 4094) {
    throw std::invalid_argument(
        fmt::format("VLAN ID {} is out of range (valid 1-4094)", *id));
  }
  return VlanID(*id);
}

// A vlan<id> name that reached this point is unambiguously a VLAN SVI
// reference, so report the VLAN-level cause instead of the generic
// port-or-interface-not-found error. Never creates the VLAN or the SVI.
[[noreturn]] void throwVlanNotResolvable(VlanID vlanId) {
  const auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  const auto& vlans = *swConfig.vlans();
  bool vlanExists = std::any_of(
      vlans.cbegin(), vlans.cend(), [vlanId](const cfg::Vlan& vlan) {
        return *vlan.id() == static_cast<int32_t>(vlanId);
      });
  if (vlanExists) {
    throw std::invalid_argument(
        fmt::format(
            "VLAN {} has no L3 interface (SVI) configured",
            static_cast<int32_t>(vlanId)));
  }
  throw std::invalid_argument(
      fmt::format(
          "VLAN {} not found in configuration", static_cast<int32_t>(vlanId)));
}

} // namespace

InterfaceList::InterfaceList(std::vector<std::string> names, bool allowMissing)
    : names_(std::move(names)) {
  // Get the PortMap from the session
  auto& portMap = ConfigSession::getInstance().getPortMap();

  // Resolve names to Intf objects
  std::vector<std::string> notFound;

  for (const auto& name : names_) {
    Intf intf(name);

    // First try to look up as a port name
    cfg::Port* port = portMap.getPort(name);
    if (port) {
      intf.setPort(port);
      // Also try to get the associated interface
      auto interfaceId = portMap.getInterfaceIdForPort(name);
      if (interfaceId) {
        cfg::Interface* interface = portMap.getInterface(*interfaceId);
        if (interface) {
          intf.setInterface(interface);
        }
      }
    } else {
      // If not found as a port, try as an interface name, then as an
      // interface ID, then as a "vlan<id>" SVI name.
      cfg::Interface* interface = portMap.getInterfaceByName(name);
      if (!interface) {
        // A purely-numeric name may be an interface ID.
        auto parsedInterfaceId = folly::tryTo<int32_t>(name);
        if (parsedInterfaceId.hasValue() && *parsedInterfaceId >= 0) {
          interface = portMap.getInterface(InterfaceID(*parsedInterfaceId));
        }
      }
      if (!interface) {
        // "vlan2001" names the SVI of VLAN 2001: the interface whose vlanID
        // matches, regardless of the interface's own (often unset) name.
        if (auto vlanId = parseVlanName(name)) {
          interface = portMap.getInterfaceForVlan(*vlanId);
          if (!interface && !allowMissing) {
            throwVlanNotResolvable(*vlanId);
          }
        }
      }
      if (interface) {
        intf.setInterface(interface);
      }
    }

    if (!intf.isValid() && !allowMissing) {
      notFound.push_back(name);
    } else {
      data_.push_back(intf);
    }
  }

  if (!notFound.empty()) {
    throw std::invalid_argument(
        "Port(s) or interface(s) not found in configuration: " +
        folly::join(", ", notFound) +
        ". Ports must exist in the hardware platform mapping and be defined "
        "in the configuration before they can be configured.");
  }
}

} // namespace facebook::fboss::utils
