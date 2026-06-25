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

const std::string kValidDeleteAttrs =
    fmt::format("loopback-mode, {}", folly::join(", ", lldpAttrNames()));

} // namespace

InterfaceDeleteConfig::InterfaceDeleteConfig(const std::vector<std::string>& v)
    // For delete, every known attribute is valueless (delete always resets to
    // default), so the known and valueless sets are identical.
    : InterfaceAttrArgsBase(
          kValuelessDeleteAttributes,
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
    // Port-level valueless reset (loopback-mode, lldp-expected-*).
    // Only report interfaces actually backed by a port; only save the
    // config if a reset was a real change (not already at default).
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
