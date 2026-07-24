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
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/cli/fboss2/commands/config/interface/InterfaceIpUtils.h"
#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"
#include "fboss/lib/config/AgentConfigUtils.h"

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
      "lookup-class",
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
    "no-shutdown, lookup-class";

// The value of the `profile` attribute if the parsed attribute list configures
// one, else nullopt. Centralized (single scan) so the InterfacesConfig
// allowMissing decision and queryClient stay in sync.
std::optional<std::string> findProfileValue(
    const std::vector<std::pair<std::string, std::string>>& attributes) {
  for (const auto& [attr, value] : attributes) {
    if (attr == "profile") {
      return value;
    }
  }
  return std::nullopt;
}

} // namespace

InterfacesConfig::InterfacesConfig(const std::vector<std::string>& v)
    : InterfaceAttrArgsBase(
          kKnownAttributes,
          kValuelessAttributes,
          "attribute",
          kValidConfigAttrs) {
  auto portNames = parseTokens(v);

  // A `profile` attribute can CREATE an absent-but-valid INTERFACE_PORT, so its
  // names are allowed to be missing from the running config at resolution time
  // (they are created later in queryClient). Without it, every name must
  // already resolve.
  const bool allowMissing = findProfileValue(getAttributes()).has_value();

  // Now resolve the port names to InterfaceList. Unless allowMissing is set,
  // this throws if any port is not found.
  interfaces_ = utils::InterfaceList(std::move(portNames), allowMissing);
}

namespace {
// Resolve a port name by logical id within the config (for output/messages).
// Callers pass ids known to be present in the config, so a missing port is a
// broken invariant and is surfaced rather than silently masked. Unnamed ports
// fall back to the agent's default naming convention ("port" + logicalID, see
// ThriftHandlerUtils.h).
std::string portNameForId(const cfg::SwitchConfig& cfg, PortID id) {
  for (const auto& p : *cfg.ports()) {
    if (PortID(*p.logicalID()) == id) {
      return p.name().has_value()
          ? *p.name()
          : folly::to<std::string>("port", *p.logicalID());
    }
  }
  throw std::runtime_error(
      fmt::format(
          "Port with logical ID {} not found in config",
          static_cast<uint32_t>(id)));
}
} // namespace

std::string applyProfileImpl(
    ProfileValidator& validator,
    cfg::SwitchConfig& swConfig,
    const utils::InterfaceList& interfaces,
    const std::string& value) {
  // Parse once (throws std::invalid_argument on a bad string) and thread the
  // enum through the rest of the flow.
  const cfg::PortProfileID requestedProfile =
      ProfileValidator::parseProfile(value);

  // 1) Pre-check absent (getPort()==nullptr) requested ports without mutating:
  //    each must resolve to a creatable INTERFACE_PORT.
  const PlatformMapping& pm = validator.getPlatformMapping();
  std::vector<std::pair<std::string, PortID>> pending;
  for (const utils::Intf& intf : interfaces) {
    if (intf.getPort() != nullptr) {
      continue;
    }
    const std::string& name = intf.name();

    PortID id;
    try {
      id = pm.getPortID(name);
    } catch (const FbossError& e) {
      throw std::invalid_argument(
          fmt::format("{} is not a valid platform port: {}", name, e.what()));
    }

    if (*pm.getPlatformPort(static_cast<int32_t>(id)).mapping()->portType() !=
        cfg::PortType::INTERFACE_PORT) {
      throw std::invalid_argument(
          fmt::format("Only interface ports can be created: {}", name));
    }

    pending.emplace_back(name, id);
  }

  // 2) Validate the whole change (present + absent ports) at the new profile.
  //    Non-mutating: throws before any port is adjusted, removed, or created.
  auto result = validator.validateProfileChange(
      swConfig, interfaces.getNames(), requestedProfile);

  // 3) Adjust EXISTING ports first (before any removal/creation), capturing the
  //    speed here from a resolved PortID so the report never needs a name
  //    lookup (which could throw) after mutation.
  cfg::PortSpeed profileSpeed = cfg::PortSpeed::DEFAULT;
  bool speedResolved = false;
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      continue;
    }
    PortID portId(static_cast<uint32_t>(*port->logicalID()));
    profileSpeed = validator.getProfileSpeed(portId, requestedProfile);
    speedResolved = true;
    port->profileID() = requestedProfile;
    port->speed() = profileSpeed;
  }
  // A pending-only request has no existing port above; derive the speed from an
  // already-resolved pending PortID.
  if (!speedResolved && !pending.empty()) {
    profileSpeed =
        validator.getProfileSpeed(pending.front().second, requestedProfile);
  }

  // 4) Remove the subsumed ports (+ dependents); capture their names first.
  std::vector<std::string> removedNames;
  if (!result.portsToRemove.empty()) {
    for (PortID id : result.portsToRemove) {
      removedNames.push_back(portNameForId(swConfig, id));
    }
    utility::removePortsFromConfig(
        swConfig,
        std::set<PortID>(
            result.portsToRemove.begin(), result.portsToRemove.end()));
  }

  // 5) Create absent ports now that the config reflects the adjusted/removed
  //    existing ports.
  std::vector<std::string> createdNames;
  for (const auto& [name, id] : pending) {
    utility::addInterfacePortToConfig(swConfig, &pm, id, requestedProfile);
    createdNames.push_back(name);
  }

  // 6) Report.
  // Normalize to uppercase for display
  std::string upperValue = value;
  std::transform(
      upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);
  std::string out = fmt::format(
      "profile={}, speed={}", upperValue, static_cast<int64_t>(profileSpeed));
  if (!removedNames.empty()) {
    out += fmt::format(
        "; auto-removed subsumed port(s): {}", folly::join(", ", removedNames));
  }
  if (!createdNames.empty()) {
    out +=
        fmt::format("; created port(s): {}", folly::join(", ", createdNames));
  }
  return out;
}

std::string applyProfile(
    const HostInfo& hostInfo,
    const utils::InterfaceList& interfaces,
    const std::string& value) {
  // Build validator once (queries Agent + QSFP), then delegate to the testable
  // core operating on the session's config.
  ProfileValidator validator(hostInfo);
  return applyProfileImpl(
      validator,
      *ConfigSession::getInstance().getAgentConfig().sw(),
      interfaces,
      value);
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

// Port.lookupClasses drives queue-per-host (MH-NIC) neighbor classification,
// so only the CLASS_QUEUE_PER_HOST_QUEUE_* classes are configurable here.
// The other AclLookupClass members (CLASS_DROP, DST_CLASS_L3_LOCAL_*, ...)
// are assigned by the agent itself; putting one of them in a port's list
// would make LookupClassUpdater tag neighbors with an agent-reserved class.
bool isQueuePerHostClass(cfg::AclLookupClass lookupClass) {
  return lookupClass >= cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 &&
      lookupClass <= cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_9;
}

// Human-readable list of every configurable lookup class as "<id> (<name>)".
std::string validLookupClasses() {
  std::vector<std::string> entries;
  for (const auto value :
       apache::thrift::TEnumTraits<cfg::AclLookupClass>::values) {
    if (!isQueuePerHostClass(value)) {
      continue;
    }
    entries.push_back(
        fmt::format(
            "{} ({})",
            static_cast<int>(value),
            apache::thrift::util::enumNameSafe(value)));
  }
  return folly::join(", ", entries);
}

// Parses a single lookup-class token: a numeric id (e.g. "10") or an enum
// name (e.g. "CLASS_QUEUE_PER_HOST_QUEUE_0", case-insensitive). Only
// queue-per-host classes are accepted.
cfg::AclLookupClass parseLookupClassId(const std::string& token) {
  cfg::AclLookupClass lookupClass{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0};
  int32_t classId = 0;
  if (folly::tryTo<int32_t>(token).hasValue()) {
    classId = folly::to<int32_t>(token);
    lookupClass = static_cast<cfg::AclLookupClass>(classId);
    if (apache::thrift::TEnumTraits<cfg::AclLookupClass>::findName(
            lookupClass) == nullptr) {
      throw std::invalid_argument(
          fmt::format(
              "Invalid lookup-class value '{}'. Valid values: {}",
              token,
              validLookupClasses()));
    }
  } else {
    std::string tokenUpper = token;
    std::transform(
        tokenUpper.begin(),
        tokenUpper.end(),
        tokenUpper.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (!apache::thrift::TEnumTraits<cfg::AclLookupClass>::findValue(
            tokenUpper, &lookupClass)) {
      throw std::invalid_argument(
          fmt::format(
              "Invalid lookup-class value '{}': must be a numeric id or class "
              "name. Valid values: {}",
              token,
              validLookupClasses()));
    }
  }

  if (!isQueuePerHostClass(lookupClass)) {
    throw std::invalid_argument(
        fmt::format(
            "Invalid lookup-class value '{}': {} is reserved for agent use. "
            "Valid values: {}",
            token,
            apache::thrift::util::enumNameSafe(lookupClass),
            validLookupClasses()));
  }
  return lookupClass;
}

// Parses a comma-separated list of lookup classes
// (e.g. "10" or "10,11,12,13,14"). Rejects empty slots and duplicates.
std::vector<cfg::AclLookupClass> parseLookupClassList(
    const std::string& value) {
  std::vector<std::string> parts;
  folly::split(',', value, parts, /*ignoreEmpty=*/false);

  std::vector<cfg::AclLookupClass> classes;
  for (auto& part : parts) {
    part = folly::trimWhitespace(part).str();
    if (part.empty()) {
      throw std::invalid_argument(
          fmt::format(
              "Invalid lookup-class value '{}': empty id in list", value));
    }
    auto lookupClass = parseLookupClassId(part);
    if (std::find(classes.begin(), classes.end(), lookupClass) !=
        classes.end()) {
      throw std::invalid_argument(
          fmt::format(
              "Invalid lookup-class value '{}': duplicate id {}",
              value,
              static_cast<int32_t>(lookupClass)));
    }
    classes.push_back(lookupClass);
  }
  return classes;
}

// Parses a comma-separated list of AclLookupClass ids and applies it to all
// ports, replacing any existing lookupClasses list. Returns true if any port
// was modified.
bool applyLookupClass(
    const std::string& value,
    const utils::InterfaceList& interfaces) {
  auto lookupClasses = parseLookupClassList(value);

  bool changed = false;
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      port->lookupClasses() = lookupClasses;
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

  // The `profile` attribute mutates the port set (adjusts/removes existing
  // ports and creates absent ones), so it must be applied first, over the
  // ORIGINAL interfaces (which still carry pending Intfs with getPort()==
  // nullptr). Re-resolve afterwards so the remaining attributes see valid
  // pointers for freshly-created ports and no stale pointers to removed ones.
  const std::optional<std::string> profileValue = findProfileValue(attributes);

  utils::InterfaceList resolved(std::vector<std::string>{});
  if (profileValue.has_value()) {
    results.push_back(applyProfile(hostInfo, interfaces, *profileValue));
    changed = true;
    ConfigSession::getInstance().rebuildPortMap();
    resolved = utils::InterfaceList(interfaces.getNames());
  }
  const utils::InterfaceList& effectiveInterfaces =
      profileValue.has_value() ? resolved : interfaces;

  for (const auto& [attr, value] : attributes) {
    if (attr == "profile") {
      continue;
    } else if (attr == "description") {
      for (const utils::Intf& intf : effectiveInterfaces) {
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
      for (const utils::Intf& intf : effectiveInterfaces) {
        cfg::Interface* interface = intf.getInterface();
        if (interface) {
          auto& ipAddresses = *interface->ipAddresses();
          // Only add if not already present
          if (std::find(ipAddresses.begin(), ipAddresses.end(), value) ==
              ipAddresses.end()) {
            ipAddresses.push_back(value);
          }
          changed = true;
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

      for (const utils::Intf& intf : effectiveInterfaces) {
        cfg::Interface* interface = intf.getInterface();
        if (interface) {
          interface->mtu() = mtu;
          changed = true;
        }
      }
      results.push_back(fmt::format("mtu={}", mtu));
    } else if (attr == "loopback-mode") {
      changed |= applyLoopbackMode(value, effectiveInterfaces);
      results.push_back(fmt::format("loopback-mode={}", value));
    } else if (attr == "flow-control-rx" || attr == "flow-control-tx") {
      changed |= applyFlowControl(attr, value, effectiveInterfaces);
      results.push_back(fmt::format("{}={}", attr, value));
    } else if (auto lldpTag = lldpTagForAttr(attr); lldpTag.has_value()) {
      if (value.empty()) {
        throw std::invalid_argument(fmt::format("{} cannot be empty", attr));
      }
      for (const utils::Intf& intf : effectiveInterfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->expectedLLDPValues()[*lldpTag] = value;
          changed = true;
        }
      }
      results.push_back(fmt::format("{}=\"{}\"", attr, value));
    } else if (attr == "type") {
      changed |= applyPortType(value, effectiveInterfaces);
      results.push_back(fmt::format("type={}", value));
    } else if (attr == "lookup-class") {
      changed |= applyLookupClass(value, effectiveInterfaces);
      results.push_back(fmt::format("lookup-class={}", value));
    } else if (attr == "shutdown") {
      for (const utils::Intf& intf : effectiveInterfaces) {
        cfg::Port* port = intf.getPort();
        if (port) {
          port->state() = cfg::PortState::DISABLED;
          changed = true;
        }
      }
      results.emplace_back("state=disabled");
    } else if (attr == "no-shutdown") {
      for (const utils::Intf& intf : effectiveInterfaces) {
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
