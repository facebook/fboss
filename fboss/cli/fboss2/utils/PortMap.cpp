/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/utils/PortMap.h"

#include <glog/logging.h>

namespace facebook::fboss::utils {

PortMap::PortMap(const cfg::AgentConfig& config) {
  const auto& switchConfig = *config.sw();

  // Build the mappings in order:
  // 1. Port name <-> logical ID
  buildPortMaps(switchConfig);

  // 2. VLAN-based mappings (logicalPort <-> vlanID)
  buildVlanMaps(switchConfig);

  // 3. Interface mappings (using portID, name, or VLAN)
  buildInterfaceMaps(switchConfig);
}

void PortMap::buildPortMaps(const cfg::SwitchConfig& switchConfig) {
  for (auto& port : *switchConfig.ports()) {
    // logicalID and name are required fields, so we can dereference directly
    int32_t logicalId = *port.logicalID();
    const std::string& portName = *port.name();

    portNameToLogicalId_[portName] = logicalId;
    portLogicalIdToName_[logicalId] = portName;
    portNameToPort_[portName] = const_cast<cfg::Port*>(&port);
  }

  VLOG(2) << "Built port maps with " << portNameToLogicalId_.size() << " ports";
}

void PortMap::buildVlanMaps(const cfg::SwitchConfig& switchConfig) {
  if (!switchConfig.vlanPorts().has_value()) {
    return;
  }

  for (const auto& vlanPort : *switchConfig.vlanPorts()) {
    // vlanID and logicalPort are required fields
    int32_t vlanId = *vlanPort.vlanID();
    int32_t logicalPort = *vlanPort.logicalPort();

    portLogicalIdToVlanId_[logicalPort] = vlanId;
    vlanIdToPortLogicalId_[vlanId] = logicalPort;
  }

  VLOG(2) << "Built VLAN maps with " << vlanIdToPortLogicalId_.size()
          << " VLAN-port mappings";
}

void PortMap::buildInterfaceMaps(const cfg::SwitchConfig& switchConfig) {
  if (!switchConfig.interfaces().has_value()) {
    return;
  }

  // First pass: build interface ID to VLAN ID mapping and store interface
  // pointers
  for (auto& interface : *switchConfig.interfaces()) {
    // intfID is required
    int32_t intfId = *interface.intfID();

    // Store pointer to interface object
    interfaceIdToInterface_[intfId] = const_cast<cfg::Interface*>(&interface);

    // vlanID is optional
    if (interface.vlanID().has_value()) {
      int32_t vlanId = *interface.vlanID();
      interfaceIdToVlanId_[intfId] = vlanId;
      vlanIdToInterfaceId_[vlanId] = intfId;
    }
  }

  // Second pass: map each interface to a port and build name mapping
  for (const auto& interface : *switchConfig.interfaces()) {
    mapInterfaceToPort(interface);

    // Build interface name mapping
    int32_t intfId = *interface.intfID();
    if (interface.name().has_value()) {
      const std::string& intfName = *interface.name();
      interfaceNameToInterface_[intfName] =
          const_cast<cfg::Interface*>(&interface);
      VLOG(3) << "Mapped interface name " << intfName << " to interface "
              << intfId;
    }
  }

  VLOG(2) << "Built interface maps with " << portNameToInterfaceId_.size()
          << " port-to-interface mappings and "
          << interfaceNameToInterface_.size() << " interface name mappings";
}

void PortMap::mapInterfaceToPort(const cfg::Interface& interface) {
  // intfID is required
  int32_t intfId = *interface.intfID();
  std::optional<int32_t> portLogicalId;

  // Use portID attribute if set (optional field)
  if (interface.portID().has_value()) {
    portLogicalId = *interface.portID();
    VLOG(3) << "Interface " << intfId
            << " mapped to port via portID: " << *portLogicalId;
  } else if (interface.vlanID().has_value()) {
    // Fall back to using VLAN-based mapping (optional field)
    // The assumption here is that there is a one-to-one mapping between VLAN
    // and port, for physical interfaces. This is true for most platforms.
    int32_t vlanId = *interface.vlanID();
    auto it = vlanIdToPortLogicalId_.find(vlanId);
    if (it != vlanIdToPortLogicalId_.end()) {
      portLogicalId = it->second;
      VLOG(3) << "Interface " << intfId
              << " mapped to port via VLAN: " << vlanId << " -> "
              << *portLogicalId;
    }
  }

  // If we found a port logical ID, create the final mapping
  if (portLogicalId) {
    auto portNameIt = portLogicalIdToName_.find(*portLogicalId);
    if (portNameIt != portLogicalIdToName_.end()) {
      const std::string& portName = portNameIt->second;

      // Validate that this port is not already mapped to a different interface
      auto existingMapping = portNameToInterfaceId_.find(portName);
      if (existingMapping != portNameToInterfaceId_.end()) {
        throw std::runtime_error(
            "Port " + portName + " (logical ID " +
            std::to_string(*portLogicalId) +
            ") is already mapped to interface " +
            std::to_string(existingMapping->second) +
            ". Cannot map it to interface " + std::to_string(intfId) +
            " as well.");
      }

      portNameToInterfaceId_[portName] = intfId;
      interfaceIdToPortName_[intfId] = portName;
      VLOG(3) << "Final mapping: port " << portName << " <-> interface "
              << intfId;
    } else {
      VLOG(2) << "Could not find port name for interface " << intfId
              << " with logical ID " << *portLogicalId;
    }
  } else {
    VLOG(3) << "Could not map interface " << intfId << " to any port";
  }
}

std::optional<int32_t> PortMap::getInterfaceIdForPort(
    const std::string& portName) const {
  auto it = portNameToInterfaceId_.find(portName);
  if (it != portNameToInterfaceId_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<std::string> PortMap::getPortNameForInterface(
    int32_t interfaceId) const {
  auto it = interfaceIdToPortName_.find(interfaceId);
  if (it != interfaceIdToPortName_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<int32_t> PortMap::getPortLogicalId(
    const std::string& portName) const {
  auto it = portNameToLogicalId_.find(portName);
  if (it != portNameToLogicalId_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool PortMap::hasPort(const std::string& portName) const {
  return portNameToLogicalId_.find(portName) != portNameToLogicalId_.end();
}

bool PortMap::hasInterface(int32_t interfaceId) const {
  return interfaceIdToPortName_.find(interfaceId) !=
      interfaceIdToPortName_.end();
}

std::vector<std::string> PortMap::getAllPortNames() const {
  std::vector<std::string> portNames;
  portNames.reserve(portNameToLogicalId_.size());
  for (const auto& [portName, _] : portNameToLogicalId_) {
    portNames.push_back(portName);
  }
  return portNames;
}

std::vector<int32_t> PortMap::getAllInterfaceIds() const {
  std::vector<int32_t> interfaceIds;
  interfaceIds.reserve(interfaceIdToPortName_.size());
  for (const auto& [interfaceId, _] : interfaceIdToPortName_) {
    interfaceIds.push_back(interfaceId);
  }
  return interfaceIds;
}

cfg::Port* PortMap::getPort(const std::string& portName) const {
  auto it = portNameToPort_.find(portName);
  if (it != portNameToPort_.end()) {
    return it->second;
  }
  return nullptr;
}

cfg::Interface* PortMap::getInterfaceByName(
    const std::string& interfaceName) const {
  auto it = interfaceNameToInterface_.find(interfaceName);
  if (it != interfaceNameToInterface_.end()) {
    return it->second;
  }
  return nullptr;
}

cfg::Interface* PortMap::getInterface(int32_t interfaceId) const {
  auto it = interfaceIdToInterface_.find(interfaceId);
  if (it != interfaceIdToInterface_.end()) {
    return it->second;
  }
  return nullptr;
}

} // namespace facebook::fboss::utils
