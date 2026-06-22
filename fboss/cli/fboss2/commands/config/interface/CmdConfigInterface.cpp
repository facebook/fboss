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
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/interface/InterfaceIpUtils.h"
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
      "ip-address",
      "ipv6-address",
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
    "description, mtu, ip-address, ipv6-address, profile, loopback-mode, "
    "flow-control-rx, flow-control-tx, lldp-expected-*, type, shutdown, "
    "no-shutdown";

// Result of applying a profile change: the user-facing message plus the
// minimum agent action level needed to make the change take effect safely.
// Profile changes go through the BCM SAI flexport path which only applies
// reliably on a cold boot (the live-delta recreate path leaves the port
// macro in a half-configured state — see
// SaiPortManager::changePortByRecreate). Cold boot reconstructs SAI ports
// from the on-disk config, where the SDK accepts the new layout.
struct ProfileApplyResult {
  std::string message;
  cli::ConfigActionLevel actionLevel = cli::ConfigActionLevel::HITLESS;
};

} // namespace

InterfacesConfig::InterfacesConfig(const std::vector<std::string>& v)
    : InterfaceAttrArgsBase(
          kKnownAttributes,
          kValuelessAttributes,
          "attribute",
          kValidConfigAttrs) {
  auto portNames = parseTokens(v);

  // Now resolve the port names to InterfaceList
  // This will throw if any port is not found
  interfaces_ = utils::InterfaceList(std::move(portNames));
}

ProfileApplyResult applyProfile(
    const HostInfo& hostInfo,
    const utils::InterfaceList& interfaces,
    const std::string& value) {
  // Phase 0: Parse profile string — fast fail for completely invalid strings.
  cfg::PortProfileID requestedProfile = ProfileValidator::parseProfile(value);

  // Detect a no-op re-apply: every target port already has this profile.
  // We skip the coldboot in that case — the on-disk and live configs are
  // already equivalent, so HITLESS (reloadConfig) suffices.
  bool isNoOp = true;
  for (const utils::Intf& intf : interfaces) {
    const cfg::Port* port = intf.getPort();
    if (port && *port->profileID() != requestedProfile) {
      isNoOp = false;
      break;
    }
  }

  // Phase 1: Build validator once (queries Agent + QSFP).
  ProfileValidator validator(hostInfo);

  // Phase 2: Build the full change list without mutating anything yet.
  std::vector<ProfileValidator::ProfileChange> changes;
  for (const utils::Intf& intf : interfaces) {
    const cfg::Port* port = intf.getPort();
    if (!port) {
      continue;
    }
    changes.push_back(
        {apache::thrift::can_throw(*port->name()), requestedProfile});
  }

  // No interface in the list resolved to a port — nothing to apply. Fail
  // loudly rather than silently "succeeding" with an empty batch (which
  // validateBatch() would pass vacuously, returning speed=0 / HITLESS).
  if (changes.empty()) {
    throw std::runtime_error(
        "No port found for the specified interface(s); profile change cannot "
        "be applied");
  }

  // Phase 3: Validate all — NO mutation yet.
  // validateBatch() checks per-port profile membership AND parent-subsumption
  // (a port whose parent's profile subsumes it cannot be configured) in one
  // pass.
  auto result = validator.validateBatch(changes);

  // If any errors, throw with the full aggregated message.  Zero mutations
  // have occurred at this point — atomicity is guaranteed.  Check this before
  // emitting warnings: a rejected batch applies nothing, so its subsumed-port
  // "will be disabled" notices would be misleading.
  if (!result.errors.empty()) {
    throw std::invalid_argument(folly::join("\n", result.errors));
  }

  // Emit warnings (e.g. subsumed-port notices) to stderr — the batch is valid
  // and about to be applied.
  for (const auto& w : result.warnings) {
    std::cerr << "Warning: " << w << std::endl;
  }

  // Phase 4: Mutation — only reached when the whole batch is valid.
  // Speed is a property of the profile, not the port — look it up once.
  cfg::PortSpeed profileSpeed = cfg::PortSpeed::DEFAULT;
  for (const utils::Intf& intf : interfaces) {
    const cfg::Port* port = intf.getPort();
    if (port) {
      PortID portId(static_cast<uint32_t>(*port->logicalID()));
      profileSpeed = validator.getProfileSpeed(portId, requestedProfile);
      break;
    }
  }

  // getProfileSpeed() returns DEFAULT (0) when PlatformMapping has no
  // profile-config entry for this port/profile. Writing speed=0 to the config
  // makes the agent reject it on apply, so fail here instead.
  if (profileSpeed == cfg::PortSpeed::DEFAULT) {
    throw std::runtime_error(
        fmt::format(
            "Could not determine speed for profile '{}' from PlatformMapping",
            value));
  }

  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      continue;
    }
    port->profileID() = requestedProfile;
    port->speed() = profileSpeed;
  }

  // Phase 5: remove subsumed ports from config. When a profile absorbs
  // neighbor lanes (PlatformPortConfig.subsumedPorts), those ports must be
  // absent from the agent config; leaving them present (even DISABLED)
  // aborts the sw agent on apply. Mirrors patcher.py:remove_subsumed_ports
  // (filter ports, vlanPorts, interfaces by subsumed-port IDs).
  if (!result.subsumedPorts.empty()) {
    auto& agentConfig = ConfigSession::getInstance().getAgentConfig();
    std::set<int32_t> subsumedIds;
    for (const auto& pid : result.subsumedPorts) {
      subsumedIds.insert(static_cast<int32_t>(pid));
    }
    auto isSubsumed = [&](int32_t portId) {
      return subsumedIds.count(portId) > 0;
    };

    auto& ports = *agentConfig.sw()->ports();
    ports.erase(
        std::remove_if(
            ports.begin(),
            ports.end(),
            [&](const cfg::Port& p) { return isSubsumed(*p.logicalID()); }),
        ports.end());

    auto& vlanPorts = *agentConfig.sw()->vlanPorts();
    vlanPorts.erase(
        std::remove_if(
            vlanPorts.begin(),
            vlanPorts.end(),
            [&](const cfg::VlanPort& vp) {
              return isSubsumed(*vp.logicalPort());
            }),
        vlanPorts.end());

    // Strip cfg::Interface entries that reference a subsumed port. Only
    // InterfaceType::PORT entries carry portID; VLAN-type interfaces are
    // untouched. Leaving an orphan PORT-type interface pointing to a
    // removed port aborts the agent on apply.
    auto& interfaces = *agentConfig.sw()->interfaces();
    interfaces.erase(
        std::remove_if(
            interfaces.begin(),
            interfaces.end(),
            [&](const cfg::Interface& intf) {
              return intf.portID().has_value() && isSubsumed(*intf.portID());
            }),
        interfaces.end());
  }

  // Normalize to uppercase for display.
  std::string upperValue = value;
  std::transform(
      upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);
  return ProfileApplyResult{
      fmt::format(
          "profile={}, speed={}",
          upperValue,
          static_cast<int64_t>(profileSpeed)),
      isNoOp ? cli::ConfigActionLevel::HITLESS
             : cli::ConfigActionLevel::AGENT_COLDBOOT};
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

  if (!interfaceConfig.hasAttributes()) {
    throw std::runtime_error(
        fmt::format(
            "Incomplete command. Either provide attributes ({}) "
            "or use a subcommand (switchport)",
            kValidConfigAttrs));
  }

  std::vector<std::string> results;
  bool changed = false;
  cli::ConfigActionLevel maxActionLevel = cli::ConfigActionLevel::HITLESS;
  // Process each attribute
  for (const auto& [attr, value] : attributes) {
    if (attr == "description") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->description() = value;
          changed = true;
        }
      }
      results.push_back(fmt::format("description=\"{}\"", value));
    } else if (attr == "ip-address" || attr == "ipv6-address") {
      validateInterfaceIpAttr(attr, value);

      // Add IP address to all interfaces
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* interface = intf.getInterface();
        if (interface) {
          auto& ipAddresses = *interface->ipAddresses();
          // Only add if not already present
          if (std::find(ipAddresses.begin(), ipAddresses.end(), value) ==
              ipAddresses.end()) {
            ipAddresses.push_back(value);
          }
        }
      }
      results.push_back(fmt::format("{}={}", attr, value));
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
          changed = true;
        }
      }
      results.push_back(fmt::format("mtu={}", mtu));
    } else if (attr == "profile") {
      auto profileResult = applyProfile(hostInfo, interfaces, value);
      results.push_back(std::move(profileResult.message));
      changed = true;
      if (static_cast<int>(profileResult.actionLevel) >
          static_cast<int>(maxActionLevel)) {
        maxActionLevel = profileResult.actionLevel;
      }
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
    ConfigSession::getInstance().saveConfig(
        cli::ServiceType::AGENT, maxActionLevel);
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
