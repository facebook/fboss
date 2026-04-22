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
#include <cctype>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

namespace {

// Set of known delete attribute names
const std::unordered_set<std::string> kKnownDeleteAttributes = {
    "loopback-mode",
    "lldp-expected-value",
    "lldp-expected-chassis",
    "lldp-expected-ttl",
    "lldp-expected-port-desc",
    "lldp-expected-system-name",
    "lldp-expected-system-desc",
};

// Maps an lldp-expected-* attribute name to its LLDPTag enum value.
// Returns nullopt if attr is not a recognised lldp-expected-* attribute.
std::optional<cfg::LLDPTag> lldpTagForAttr(const std::string& attr) {
  static const std::unordered_map<std::string, cfg::LLDPTag> kAttrToTag = {
      {"lldp-expected-value", cfg::LLDPTag::PORT},
      {"lldp-expected-chassis", cfg::LLDPTag::CHASSIS},
      {"lldp-expected-ttl", cfg::LLDPTag::TTL},
      {"lldp-expected-port-desc", cfg::LLDPTag::PORT_DESC},
      {"lldp-expected-system-name", cfg::LLDPTag::SYSTEM_NAME},
      {"lldp-expected-system-desc", cfg::LLDPTag::SYSTEM_DESC},
  };
  auto it = kAttrToTag.find(attr);
  return it != kAttrToTag.end() ? std::make_optional(it->second) : std::nullopt;
}

} // namespace

bool InterfaceDeleteAttrs::isKnownAttribute(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return kKnownDeleteAttributes.find(lower) != kKnownDeleteAttributes.end();
}

InterfaceDeleteAttrs::InterfaceDeleteAttrs(std::vector<std::string> v)
    : interfaces_(std::vector<std::string>{}) {
  if (v.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // Find where port names end and attributes begin.
  // Ports are all tokens before the first known attribute name.
  size_t attrStart = v.size();
  for (size_t i = 0; i < v.size(); ++i) {
    if (isKnownAttribute(v[i])) {
      attrStart = i;
      break;
    }
  }

  // Must have at least one port name
  if (attrStart == 0) {
    throw std::invalid_argument(
        fmt::format(
            "No interface name provided. First token '{}' is an attribute name.",
            v[0]));
  }

  // Extract port names
  std::vector<std::string> portNames(v.begin(), v.begin() + attrStart);

  // If no known attribute was found as the boundary, check whether any
  // "port name" token looks like a mistyped attribute (contains '-' but no
  // '/').  Interface names always contain '/' (e.g. "eth1/1/1"); attribute
  // names never do.
  if (attrStart == v.size()) {
    for (const auto& tok : portNames) {
      if (tok.find('-') != std::string::npos &&
          tok.find('/') == std::string::npos) {
        throw std::invalid_argument(
            fmt::format(
                "Unknown delete attribute '{}'. Valid attributes are: "
                "loopback-mode, lldp-expected-value, lldp-expected-chassis, "
                "lldp-expected-ttl, lldp-expected-port-desc, "
                "lldp-expected-system-name, lldp-expected-system-desc",
                tok));
      }
    }
  }

  // Parse remaining tokens as attribute names (all valueless for delete)
  for (size_t i = attrStart; i < v.size(); ++i) {
    const std::string& attr = v[i];
    if (!isKnownAttribute(attr)) {
      throw std::invalid_argument(
          fmt::format(
              "Unknown delete attribute '{}'. Valid attributes are: "
              "loopback-mode, lldp-expected-value, lldp-expected-chassis, "
              "lldp-expected-ttl, lldp-expected-port-desc, "
              "lldp-expected-system-name, lldp-expected-system-desc",
              attr));
    }
    std::string attrLower = attr;
    std::transform(
        attrLower.begin(), attrLower.end(), attrLower.begin(), ::tolower);
    attributes_.push_back(attrLower);
  }

  // Resolve port names to InterfaceList
  interfaces_ = utils::InterfaceList(std::move(portNames));
}

CmdDeleteInterfaceTraits::RetType CmdDeleteInterface::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& deleteAttrs) {
  const auto& interfaces = deleteAttrs.getInterfaces();
  const auto& attrs = deleteAttrs.getAttributes();

  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // If no attributes provided, this is a pass-through to subcommands
  if (!deleteAttrs.hasAttributes()) {
    throw std::runtime_error(
        "Incomplete command. Specify attribute(s) to reset: "
        "loopback-mode, lldp-expected-value, lldp-expected-chassis, "
        "lldp-expected-ttl, lldp-expected-port-desc, "
        "lldp-expected-system-name, lldp-expected-system-desc");
  }

  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      // Idempotent: silently skip interfaces not backed by a port
      continue;
    }
    for (const auto& attr : attrs) {
      if (attr == "loopback-mode") {
        port->loopbackMode() = cfg::PortLoopbackMode::NONE;
      } else if (auto tag = lldpTagForAttr(attr); tag.has_value()) {
        port->expectedLLDPValues()->erase(*tag);
      }
    }
  }

  ConfigSession::getInstance().saveConfig();

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  std::string attrList = folly::join(", ", attrs);
  return fmt::format(
      "Successfully reset attribute(s) [{}] for interface(s): {}",
      attrList,
      interfaceList);
}

void CmdDeleteInterface::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits>::run();

} // namespace facebook::fboss
