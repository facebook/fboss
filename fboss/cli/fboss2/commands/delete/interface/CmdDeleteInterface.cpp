/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/interface/InterfaceIpUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

namespace {

// Valueless delete attributes: reset a port/interface attribute to default.
// The lldp-expected-* names come from the shared lldpAttrToTag() list so the
// config and delete commands cannot drift apart.
const std::unordered_set<std::string> kValuelessDeleteAttributes = [] {
  std::unordered_set<std::string> attrs = {"loopback-mode"};
  for (const auto& name : lldpAttrNames()) {
    attrs.insert(name);
  }
  return attrs;
}();

// All known attributes: valueless resets + valued IP address removals.
const std::unordered_set<std::string> kKnownDeleteAttributes = [] {
  std::unordered_set<std::string> attrs = kValuelessDeleteAttributes;
  attrs.insert("ip-address");
  attrs.insert("ipv6-address");
  return attrs;
}();

const std::string kValidDeleteAttrs = fmt::format(
    "loopback-mode, {}, ip-address, ipv6-address",
    folly::join(", ", lldpAttrNames()));

} // namespace

InterfaceDeleteConfig::InterfaceDeleteConfig(const std::vector<std::string>& v)
    : InterfaceAttrArgsBase(
          kKnownDeleteAttributes,
          kValuelessDeleteAttributes,
          "delete attribute",
          kValidDeleteAttrs) {
  auto portNames = parseTokens(v);

  // If no known attribute delimited the port list, catch a mistyped attribute:
  // interface names always contain '/' (e.g. "eth1/1/1"); attribute names never
  // do but do contain '-'.
  if (attributes_.empty()) {
    for (const auto& tok : portNames) {
      if (tok.find('-') != std::string::npos &&
          tok.find('/') == std::string::npos) {
        throw std::invalid_argument(
            fmt::format(
                "Unknown delete attribute '{}'. Valid attributes are: {}",
                tok,
                kValidDeleteAttrs));
      }
    }
  }

  // Validate ip-address / ipv6-address values as CIDR networks.
  for (const auto& [attr, value] : attributes_) {
    if (attr == "ip-address" || attr == "ipv6-address") {
      validateInterfaceIpAttr(attr, value);
    }
  }

  // Resolve port names to InterfaceList (throws if any port is not found).
  interfaces_ = utils::InterfaceList(std::move(portNames));
}

CmdDeleteInterfaceTraits::RetType CmdDeleteInterface::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& deleteConfig) {
  const auto& interfaces = deleteConfig.getInterfaces();
  const auto& attributes = deleteConfig.getAttributes();

  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // No attributes => pass-through to subcommands (e.g. ipv6 ndp).
  if (attributes.empty()) {
    throw std::runtime_error(
        fmt::format(
            "Incomplete command. Specify attribute(s) to delete/reset ({}) "
            "or use a subcommand (e.g. ipv6 ndp)",
            kValidDeleteAttrs));
  }

  std::vector<std::string> results;
  bool changed = false;

  for (const auto& [attr, value] : attributes) {
    if (attr == "ip-address" || attr == "ipv6-address") {
      // Remove a specific IP address from each interface's ipAddresses list.
      bool expectV6 = (attr == "ipv6-address");
      std::vector<std::string> doneNames;
      std::vector<std::string> missingNames;
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (!iface) {
          missingNames.push_back(intf.name());
          continue;
        }
        auto& ipAddresses = *iface->ipAddresses();
        auto it = std::find(ipAddresses.begin(), ipAddresses.end(), value);
        if (it != ipAddresses.end()) {
          ipAddresses.erase(it);
          changed = true;
          doneNames.push_back(intf.name());
        } else {
          results.push_back(
              fmt::format(
                  "{} {} not configured on interface {}",
                  expectV6 ? "IPv6 address" : "IP address",
                  value,
                  intf.name()));
        }
      }
      if (!doneNames.empty()) {
        results.push_back(
            fmt::format(
                "Successfully removed {} {} from interface(s): {}",
                expectV6 ? "IPv6 address" : "IP address",
                value,
                folly::join(", ", doneNames)));
      }
      if (!missingNames.empty()) {
        results.push_back(
            fmt::format(
                "No interface config found for: {}",
                folly::join(", ", missingNames)));
      }
    } else {
      // Port-level valueless reset (loopback-mode, lldp-expected-*).
      std::vector<std::string> resetNames;
      std::vector<std::string> skippedNames;
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (!port) {
          skippedNames.push_back(intf.name());
          continue;
        }
        if (attr == "loopback-mode") {
          if (*port->loopbackMode() != cfg::PortLoopbackMode::NONE) {
            port->loopbackMode() = cfg::PortLoopbackMode::NONE;
            changed = true;
          }
        } else if (auto tag = lldpTagForAttr(attr); tag.has_value()) {
          changed |= port->expectedLLDPValues()->erase(*tag) > 0;
        }
        resetNames.push_back(intf.name());
      }

      if (resetNames.empty()) {
        results.push_back(
            fmt::format(
                "Attribute '{}' not reset: no port found for interface(s) {}",
                attr,
                folly::join(", ", skippedNames)));
      } else if (skippedNames.empty()) {
        results.push_back(
            fmt::format(
                "Successfully reset attribute '{}' for interface(s): {}",
                attr,
                folly::join(", ", resetNames)));
      } else {
        results.push_back(
            fmt::format(
                "Successfully reset attribute '{}' for interface(s): {}; "
                "no port found for {}",
                attr,
                folly::join(", ", resetNames),
                folly::join(", ", skippedNames)));
      }
    }
  }

  if (changed) {
    ConfigSession::getInstance().saveConfig();
  }

  return folly::join("\n", results);
}

void CmdDeleteInterface::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits>::run();

} // namespace facebook::fboss
