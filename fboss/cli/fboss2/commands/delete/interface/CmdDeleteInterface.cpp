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

#include <fmt/format.h>
#include <folly/IPAddress.h>
#include <folly/String.h>
#include <algorithm>
#include <unordered_set>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

namespace {
const std::unordered_set<std::string> kKnownAttributes = {
    "ip-address",
    "ipv6-address",
};
} // namespace

bool InterfaceDeleteConfig::isKnownAttribute(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return kKnownAttributes.find(lower) != kKnownAttributes.end();
}

InterfaceDeleteConfig::InterfaceDeleteConfig(std::vector<std::string> v) {
  // Expected format: <interface> <attribute> <value>
  // e.g., "eth1/1/1 ip-address 10.0.0.1/24"
  //
  // Note: Unlike config interface, we only accept ONE interface name because
  // an IP address can only be configured on one interface within a VRF.
  if (v.size() < 3) {
    throw std::invalid_argument("Expected <interface> <attribute> <value>");
  }

  interfaceName_ = v[0];
  attribute_ = v[1];
  value_ = v[2];

  // Check if there are extra arguments (user trying to specify multiple
  // interfaces)
  if (v.size() > 3) {
    // Check if v[3] looks like an interface name (not starting with -)
    if (!v[3].empty() && v[3][0] != '-') {
      throw std::invalid_argument(
          "Cannot delete IP address from multiple interfaces. "
          "An IP address can only exist on one interface within a VRF. "
          "Please specify only one interface.");
    }
    // Otherwise it might be extra junk arguments
    throw std::invalid_argument(
        fmt::format(
            "Expected <interface> <attribute> <value>, got {} arguments",
            v.size()));
  }

  // Validate attribute is known
  if (!isKnownAttribute(attribute_)) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown attribute '{}'. Valid attributes are: ip-address, ipv6-address",
            attribute_));
  }

  // Normalize attribute to lowercase
  std::transform(
      attribute_.begin(), attribute_.end(), attribute_.begin(), ::tolower);

  // Validate IP address format
  try {
    folly::IPAddress::createNetwork(value_);
  } catch (const std::exception& e) {
    throw std::invalid_argument(
        fmt::format("Invalid IP address '{}': {}", value_, e.what()));
  }
}

// Delete interface implementation
CmdDeleteInterfaceTraits::RetType CmdDeleteInterface::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& deleteConfig) {
  const std::string& interfaceName = deleteConfig.getInterfaceName();
  const std::string& attribute = deleteConfig.getAttribute();
  const std::string& ipAddress = deleteConfig.getValue();

  // Validate IP address version matches attribute
  auto [ip, prefixLen] = folly::IPAddress::createNetwork(ipAddress);
  bool expectV6 = (attribute == "ipv6-address");

  if (expectV6 && !ip.isV6()) {
    throw std::invalid_argument(
        fmt::format(
            "Expected IPv6 address for 'ipv6-address', got IPv4: {}",
            ipAddress));
  }
  if (!expectV6 && ip.isV6()) {
    throw std::invalid_argument(
        fmt::format(
            "Expected IPv4 address for 'ip-address', got IPv6: {}", ipAddress));
  }

  // Get the interface using InterfaceList
  utils::InterfaceList interfaceList({interfaceName});
  if (interfaceList.data().empty() || !interfaceList.data()[0].getInterface()) {
    throw std::runtime_error(
        fmt::format("Interface '{}' not found", interfaceName));
  }

  cfg::Interface* interface = interfaceList.data()[0].getInterface();
  auto& ipAddresses = *interface->ipAddresses();

  // Find and remove the IP address
  auto it = std::find(ipAddresses.begin(), ipAddresses.end(), ipAddress);
  if (it == ipAddresses.end()) {
    return fmt::format(
        "{} {} not configured on interface {}",
        expectV6 ? "IPv6 address" : "IP address",
        ipAddress,
        interfaceName);
  }

  ipAddresses.erase(it);
  ConfigSession::getInstance().saveConfig();

  return fmt::format(
      "Successfully deleted {} {} from interface {}",
      expectV6 ? "IPv6 address" : "IP address",
      ipAddress,
      interfaceName);
}

void CmdDeleteInterface::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits>::run();

} // namespace facebook::fboss
