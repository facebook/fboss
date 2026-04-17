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
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

namespace {
// Set of known attribute names (lowercase for case-insensitive comparison)
const std::unordered_set<std::string> kKnownAttributes = {
    "description",
    "mtu",
    "profile",
    "loopback-mode",
    "flow-control-rx",
    "flow-control-tx",
    "lldp-expected-value",
    "lldp-expected-chassis",
    "lldp-expected-ttl",
    "lldp-expected-port-desc",
    "lldp-expected-system-name",
    "lldp-expected-system-desc",
    "type",
    "shutdown",
    "no-shutdown",
};

// Attributes that take no value token (boolean/action flags)
const std::unordered_set<std::string> kValuelessAttributes = {
    "shutdown",
    "no-shutdown",
};

} // namespace

bool InterfacesConfig::isKnownAttribute(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return kKnownAttributes.find(lower) != kKnownAttributes.end();
}

InterfacesConfig::InterfacesConfig(std::vector<std::string> v)
    : interfaces_(std::vector<std::string>{}) {
  if (v.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // Find where port names end and attributes begin
  // Ports are all tokens before the first known attribute name
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

  // Parse attribute-value pairs (valueless attributes consume no value token)
  for (size_t i = attrStart; i < v.size();) {
    const std::string& attr = v[i];

    if (!isKnownAttribute(attr)) {
      throw std::invalid_argument(
          fmt::format(
              "Unknown attribute '{}'. Valid attributes are: description, mtu, profile, "
              "loopback-mode, flow-control-rx, flow-control-tx, lldp-expected-*, "
              "type, shutdown, no-shutdown",
              attr));
    }

    // Normalize attribute name to lowercase
    std::string attrLower = attr;
    std::transform(
        attrLower.begin(), attrLower.end(), attrLower.begin(), ::tolower);

    // Valueless attributes (e.g. shutdown / no-shutdown) take no value token
    if (kValuelessAttributes.count(attrLower)) {
      attributes_.emplace_back(attrLower, "");
      ++i;
      continue;
    }

    if (i + 1 >= v.size()) {
      throw std::invalid_argument(
          fmt::format("Missing value for attribute '{}'", attr));
    }

    const std::string& value = v[i + 1];

    // Check if "value" is actually another attribute name (user forgot value)
    if (isKnownAttribute(value)) {
      throw std::invalid_argument(
          fmt::format(
              "Missing value for attribute '{}'. Got another attribute '{}' instead.",
              attr,
              value));
    }

    attributes_.emplace_back(attrLower, value);
    i += 2;
  }

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

  // PROFILE_DEFAULT: skip hardware validation, reset both fields to defaults
  if (requestedProfile == cfg::PortProfileID::PROFILE_DEFAULT) {
    for (const utils::Intf& intf : interfaces) {
      cfg::Port* port = intf.getPort();
      if (port) {
        port->profileID() = cfg::PortProfileID::PROFILE_DEFAULT;
        port->speed() = cfg::PortSpeed::DEFAULT;
      }
    }
    return "profile=PROFILE_DEFAULT, speed=auto";
  }

  // Non-default: validate against hardware + optics.
  // Build validator once (queries Agent + QSFP); reuse across all ports.
  ProfileValidator validator(hostInfo);

  // Speed is a property of the profile, not the port — look it up once
  // using the first port's ID to avoid rebuilding PlatformMapping per port.
  const cfg::Port* firstPort = interfaces.begin()->getPort();
  PortID firstPortId(
      firstPort ? static_cast<uint32_t>(*firstPort->logicalID()) : 0);
  cfg::PortSpeed profileSpeed =
      validator.getProfileSpeed(firstPortId, requestedProfile);

  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    const std::string& portName = *port->name();
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
// ("flow-control-rx" or "flow-control-tx").
void applyFlowControl(
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

  const bool isRx = (attr == "flow-control-rx");
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      if (isRx) {
        port->pause()->rx() = enabled;
      } else {
        port->pause()->tx() = enabled;
      }
    }
  }
}

// Validates that value is "routed-port" and applies configureAsRoutedPort.
void applyPortType(
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

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      configureAsRoutedPort(*port, swConfig);
    }
  }
}

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

CmdConfigInterfaceTraits::RetType CmdConfigInterface::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& interfaceConfig) {
  const auto& interfaces = interfaceConfig.getInterfaces();
  const auto& attributes = interfaceConfig.getAttributes();

  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  if (!interfaceConfig.hasAttributes()) {
    throw std::runtime_error(
        "Incomplete command. Either provide attributes (description, mtu, profile, "
        "loopback-mode, flow-control-rx, flow-control-tx, lldp-expected-*, type, "
        "shutdown, no-shutdown) "
        "or use a subcommand (switchport)");
  }

  std::vector<std::string> results;

  for (const auto& [attr, value] : attributes) {
    if (attr == "description") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->description() = value;
        }
      }
      results.push_back(fmt::format("description=\"{}\"", value));
    } else if (attr == "mtu") {
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
        }
      }
      results.push_back(fmt::format("mtu={}", mtu));
    } else if (attr == "profile") {
      results.push_back(applyProfile(hostInfo, interfaces, value));
    } else if (attr == "loopback-mode") {
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

      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->loopbackMode() = loopbackMode;
        }
      }
      results.push_back(fmt::format("loopback-mode={}", value));
    } else if (attr == "flow-control-rx" || attr == "flow-control-tx") {
      applyFlowControl(attr, value, interfaces);
      results.push_back(fmt::format("{}={}", attr, value));
    } else if (auto lldpTag = lldpTagForAttr(attr); lldpTag.has_value()) {
      if (value.empty()) {
        throw std::invalid_argument(fmt::format("{} cannot be empty", attr));
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->expectedLLDPValues()[*lldpTag] = value;
        }
      }
      results.push_back(fmt::format("{}=\"{}\"", attr, value));
    } else if (attr == "type") {
      applyPortType(value, interfaces);
      results.push_back(fmt::format("type={}", value));
    } else if (attr == "shutdown") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->state() = cfg::PortState::DISABLED;
        }
      }
      results.emplace_back("state=disabled");
    } else if (attr == "no-shutdown") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->state() = cfg::PortState::ENABLED;
        }
      }
      results.emplace_back("state=enabled");
    }
  }

  ConfigSession::getInstance().saveConfig();

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
