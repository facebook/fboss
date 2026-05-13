/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

namespace {
// Set of known attribute names (lowercase for case-insensitive comparison).
// The lldp-expected-* names come from the shared lldpAttrToTag() list so the
// config and delete commands cannot drift apart.
const std::unordered_set<std::string> kKnownAttributes = [] {
  std::unordered_set<std::string> attrs = {
      "description",
      "mtu",
      "profile",
      "loopback-mode",
      "flow-control-rx",
      "flow-control-tx",
      "type",
      "shutdown",
      "no-shutdown",
  };
  for (const auto& name : lldpAttrNames()) {
    attrs.insert(name);
  }
  return attrs;
}();

// Attributes that take no value token (boolean/action flags)
const std::unordered_set<std::string> kValuelessAttributes = {
    "shutdown",
    "no-shutdown",
};

constexpr auto kValidConfigAttrs =
    "description, mtu, profile, loopback-mode, flow-control-rx, "
    "flow-control-tx, lldp-expected-*, type, shutdown, no-shutdown";

} // namespace

bool InterfacesConfig::isKnownAttribute(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return kKnownAttributes.find(lower) != kKnownAttributes.end();
}

InterfacesConfig::InterfacesConfig(const std::vector<std::string>& v) {
  auto portNames = parseTokens(
      v,
      [](const std::string& s) { return isKnownAttribute(s); },
      [](const std::string& s) { return kValuelessAttributes.count(s) > 0; },
      "attribute",
      kValidConfigAttrs);

  // Now resolve the port names to InterfaceList
  // This will throw if any port is not found
  interfaces_ = utils::InterfaceList(std::move(portNames));
}

std::string applyProfile(
    const HostInfo& hostInfo,
    const utils::InterfaceList& interfaces,
    const std::string& value) {
  // Parse first — fast error for completely invalid strings
  cfg::PortProfileID requestedProfile = ProfileValidator::parseProfile(value);

  // Validate against hardware + optics.
  // Build validator once (queries Agent + QSFP); reuse across all ports.
  ProfileValidator validator(hostInfo);

  // Speed is a property of the profile, not the port — look it up once
  // using the first port's ID to avoid rebuilding PlatformMapping per port.
  const cfg::Port* firstPort = interfaces.begin()->getPort();
  if (!firstPort) {
    throw std::runtime_error("No port found for the specified interface");
  }
  PortID firstPortId(static_cast<uint32_t>(*firstPort->logicalID()));
  cfg::PortSpeed profileSpeed =
      validator.getProfileSpeed(firstPortId, requestedProfile);

  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      continue;
    }
    const std::string& portName = apache::thrift::can_throw(*port->name());
    validator.validateProfile(portName, value);
    port->profileID() = requestedProfile;
    port->speed() = profileSpeed;
  }

  // Normalize to uppercase for display
  std::string upperValue = value;
  std::transform(
      upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);
  return fmt::format(
      "profile={}, speed={}", upperValue, static_cast<int64_t>(profileSpeed));
}

// Configures a port as a routed (L3) port.
void configureAsRoutedPort(cfg::Port& port, cfg::SwitchConfig& swConfig) {
  port.portType() = cfg::PortType::INTERFACE_PORT;
  port.routable() = true;
  port.ingressVlan() = 0;

  const int32_t logicalId = *port.logicalID();
  auto& vps = *swConfig.vlanPorts();
  vps.erase(
      std::remove_if(
          vps.begin(),
          vps.end(),
          [logicalId](const cfg::VlanPort& vp) {
            return *vp.logicalPort() == logicalId;
          }),
      vps.end());
}

// Applies flow-control enable/disable to all ports for the given attribute
// ("flow-control-rx" or "flow-control-tx"). Returns true if any port was
// modified.
bool applyFlowControl(
    const std::string& attr,
    const std::string& value,
    const utils::InterfaceList& interfaces) {
  std::string valueLower = value;
  std::transform(
      valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);

  bool enabled = false;
  if (valueLower == "enable") {
    enabled = true;
  } else if (valueLower == "disable") {
    enabled = false;
  } else {
    throw std::invalid_argument(
        fmt::format(
            "Invalid {} value '{}'. Valid values: enable, disable",
            attr,
            value));
  }

  bool changed = false;
  const bool isRx = (attr == "flow-control-rx");
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      if (isRx) {
        port->pause()->rx() = enabled;
      } else {
        port->pause()->tx() = enabled;
      }
      changed = true;
    }
  }
  return changed;
}

// Validates that value is "routed-port" and applies configureAsRoutedPort.
// Returns true if any port was modified.
bool applyPortType(
    const std::string& value,
    const utils::InterfaceList& interfaces) {
  std::string valueLower = value;
  std::transform(
      valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);

  if (valueLower != "routed-port") {
    throw std::invalid_argument(
        fmt::format(
            "Invalid type value '{}'. Valid value: routed-port", value));
  }

  bool changed = false;
  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      configureAsRoutedPort(*port, swConfig);
      changed = true;
    }
  }
  return changed;
}

// Parses and applies loopback-mode to all ports. Returns true if any port
// was modified.
bool applyLoopbackMode(
    const std::string& value,
    const utils::InterfaceList& interfaces) {
  // Parse loopback mode (case-insensitive)
  std::string valueLower = value;
  std::transform(
      valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);

  cfg::PortLoopbackMode loopbackMode{cfg::PortLoopbackMode::NONE};
  if (valueLower == "none") {
    loopbackMode = cfg::PortLoopbackMode::NONE;
  } else if (valueLower == "phy") {
    loopbackMode = cfg::PortLoopbackMode::PHY;
  } else if (valueLower == "nif") {
    loopbackMode = cfg::PortLoopbackMode::NIF;
  } else if (valueLower == "mac") {
    loopbackMode = cfg::PortLoopbackMode::MAC;
  } else {
    throw std::invalid_argument(
        fmt::format(
            "Invalid loopback-mode value '{}'. Valid values: none, PHY, NIF, MAC",
            value));
  }

  bool changed = false;
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      port->loopbackMode() = loopbackMode;
      changed = true;
    }
  }
  return changed;
}

CmdConfigInterfaceTraits::RetType CmdConfigInterface::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& interfaceConfig) {
  const auto& interfaces = interfaceConfig.getInterfaces();
  const auto& attributes = interfaceConfig.getAttributes();

  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // If no attributes provided, this is a pass-through to subcommands
  if (!interfaceConfig.hasAttributes()) {
    throw std::runtime_error(
        fmt::format(
            "Incomplete command. Either provide attributes ({}) "
            "or use a subcommand (switchport)",
            kValidConfigAttrs));
  }

  std::vector<std::string> results;
  bool changed = false;

  // Process each attribute
  for (const auto& [attr, value] : attributes) {
    if (attr == "description") {
      // Set description for all ports
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->description() = value;
          changed = true;
        }
      }
      results.push_back(fmt::format("description=\"{}\"", value));
    } else if (attr == "mtu") {
      // Validate and set MTU for all interfaces
      int32_t mtu = 0;
      try {
        mtu = folly::to<int32_t>(value);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            fmt::format("Invalid MTU value '{}': must be an integer", value));
      }

      if (mtu < utils::kMtuMin || mtu > utils::kMtuMax) {
        throw std::invalid_argument(
            fmt::format(
                "MTU value {} is out of range. Valid range is {}-{}",
                mtu,
                utils::kMtuMin,
                utils::kMtuMax));
      }

      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* interface = intf.getInterface();
        if (interface) {
          interface->mtu() = mtu;
          changed = true;
        }
      }
      results.push_back(fmt::format("mtu={}", mtu));
    } else if (attr == "profile") {
      results.push_back(applyProfile(hostInfo, interfaces, value));
      changed = true;
    } else if (attr == "loopback-mode") {
      changed |= applyLoopbackMode(value, interfaces);
      results.push_back(fmt::format("loopback-mode={}", value));
    } else if (attr == "flow-control-rx" || attr == "flow-control-tx") {
      changed |= applyFlowControl(attr, value, interfaces);
      results.push_back(fmt::format("{}={}", attr, value));
    } else if (auto lldpTag = lldpTagForAttr(attr); lldpTag.has_value()) {
      if (value.empty()) {
        throw std::invalid_argument(fmt::format("{} cannot be empty", attr));
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->expectedLLDPValues()[*lldpTag] = value;
          changed = true;
        }
      }
      results.push_back(fmt::format("{}=\"{}\"", attr, value));
    } else if (attr == "type") {
      changed |= applyPortType(value, interfaces);
      results.push_back(fmt::format("type={}", value));
    } else if (attr == "shutdown") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->state() = cfg::PortState::DISABLED;
          changed = true;
        }
      }
      results.emplace_back("state=disabled");
    } else if (attr == "no-shutdown") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->state() = cfg::PortState::ENABLED;
          changed = true;
        }
      }
      results.emplace_back("state=enabled");
    }
  }

  // Save the updated config (skip the write/commit cycle if no port or
  // interface was actually modified).
  if (changed) {
    ConfigSession::getInstance().saveConfig();
  }

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  std::string attrList = folly::join(", ", results);
  return fmt::format(
      "Successfully configured interface(s) {}: {}", interfaceList, attrList);
}

void CmdConfigInterface::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdConfigInterface, CmdConfigInterfaceTraits>::run();

} // namespace facebook::fboss
