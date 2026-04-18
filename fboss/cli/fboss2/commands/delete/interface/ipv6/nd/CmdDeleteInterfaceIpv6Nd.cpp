/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/interface/ipv6/nd/CmdDeleteInterfaceIpv6Nd.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

namespace {

// All known attribute names for `delete interface <intf> ipv6 nd`
const std::unordered_set<std::string> kKnownNdDeleteAttrs = {
    "ra-interval",
    "ra-lifetime",
    "hop-limit",
    "prefix-valid-lifetime",
    "prefix-preferred-lifetime",
    "managed-config-flag",
    "other-config-flag",
    "ra-address",
};

} // namespace

// ---------------------------------------------------------------------------
// NdpDeleteAttrs constructor — validates token list against known attr names
// ---------------------------------------------------------------------------

NdpDeleteAttrs::NdpDeleteAttrs(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "No NDP attribute provided. Valid attributes: " +
        folly::join(", ", kKnownNdDeleteAttrs));
  }

  for (size_t i = 0; i < v.size(); ++i) {
    std::string attrLower = v[i];
    std::transform(
        attrLower.begin(), attrLower.end(), attrLower.begin(), ::tolower);

    if (kKnownNdDeleteAttrs.find(attrLower) == kKnownNdDeleteAttrs.end()) {
      throw std::invalid_argument(
          fmt::format(
              "Unknown nd attribute '{}'. Valid attributes: {}",
              v[i],
              folly::join(", ", kKnownNdDeleteAttrs)));
    }

    attributes_.push_back(attrLower);
  }
}

// ---------------------------------------------------------------------------
// CmdDeleteInterfaceIpv6Nd::queryClient
// ---------------------------------------------------------------------------

CmdDeleteInterfaceIpv6NdTraits::RetType CmdDeleteInterfaceIpv6Nd::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& ndpAttrs) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  for (const utils::Intf& intf : interfaces) {
    cfg::Interface* iface = intf.getInterface();
    if (!iface || !iface->ndp().has_value()) {
      // Idempotent: silently skip interfaces with no NDP config
      continue;
    }

    for (const auto& attr : ndpAttrs.getAttributes()) {
      if (attr == "ra-interval") {
        iface->ndp()->routerAdvertisementSeconds() = 0;
      } else if (attr == "hop-limit") {
        iface->ndp()->curHopLimit() = 255;
      } else if (attr == "ra-lifetime") {
        iface->ndp()->routerLifetime() = 1800;
      } else if (attr == "prefix-valid-lifetime") {
        iface->ndp()->prefixValidLifetimeSeconds() = 2592000;
      } else if (attr == "prefix-preferred-lifetime") {
        iface->ndp()->prefixPreferredLifetimeSeconds() = 604800;
      } else if (attr == "managed-config-flag") {
        iface->ndp()->routerAdvertisementManagedBit() = false;
      } else if (attr == "other-config-flag") {
        iface->ndp()->routerAdvertisementOtherBit() = false;
      } else if (attr == "ra-address") {
        iface->ndp()->routerAddress().reset();
      }
    }
  }

  ConfigSession::getInstance().saveConfig();

  return fmt::format(
      "Successfully reset interface(s) {}: {}",
      folly::join(", ", interfaces.getNames()),
      folly::join(", ", ndpAttrs.getAttributes()));
}

void CmdDeleteInterfaceIpv6Nd::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdDeleteInterfaceIpv6Nd, CmdDeleteInterfaceIpv6NdTraits>::run();

} // namespace facebook::fboss
