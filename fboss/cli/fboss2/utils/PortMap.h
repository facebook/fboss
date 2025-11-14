/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "fboss/agent/gen-cpp2/agent_config_types.h"

namespace facebook::fboss::utils {

/**
 * PortMap provides a mapping between port names and interface IDs.
 *
 * This class builds a comprehensive mapping by analyzing the agent
 * configuration:
 * 1. Maps port names to port logical IDs
 * 2. Maps interfaces to ports using:
 *    - Direct portID attribute (if set)
 *    - VLAN-based mapping (port logicalID -> vlanID -> interface with same
 * vlanID)
 *
 * This allows users to reference interfaces by their port names (e.g.,
 * "eth1/20/1") instead of interface IDs (e.g., "interface-2039").
 */
class PortMap {
 public:
  /**
   * Build the port-to-interface mapping from an agent configuration.
   *
   * @param config The agent configuration containing ports, interfaces, and
   * vlanPorts
   */
  explicit PortMap(const cfg::AgentConfig& config);

  /**
   * Get the interface ID for a given port name.
   *
   * @param portName The name of the port (e.g., "eth1/20/1")
   * @return The interface ID if found, std::nullopt otherwise
   */
  std::optional<int32_t> getInterfaceIdForPort(
      const std::string& portName) const;

  /**
   * Get the port name for a given interface ID.
   *
   * @param interfaceId The interface ID
   * @return The port name if found, std::nullopt otherwise
   */
  std::optional<std::string> getPortNameForInterface(int32_t interfaceId) const;

  /**
   * Get the port logical ID for a given port name.
   *
   * @param portName The name of the port
   * @return The port logical ID if found, std::nullopt otherwise
   */
  std::optional<int32_t> getPortLogicalId(const std::string& portName) const;

  /**
   * Check if a port name exists in the configuration.
   *
   * @param portName The name of the port
   * @return true if the port exists, false otherwise
   */
  bool hasPort(const std::string& portName) const;

  /**
   * Check if an interface ID exists in the configuration.
   *
   * @param interfaceId The interface ID
   * @return true if the interface exists, false otherwise
   */
  bool hasInterface(int32_t interfaceId) const;

  /**
   * Get all port names in the configuration.
   *
   * @return Vector of all port names
   */
  std::vector<std::string> getAllPortNames() const;

  /**
   * Get all interface IDs in the configuration.
   *
   * @return Vector of all interface IDs
   */
  std::vector<int32_t> getAllInterfaceIds() const;

  /**
   * Get a pointer to the Port object for a given port name.
   *
   * @param portName The name of the port
   * @return Pointer to the Port object if found, nullptr otherwise
   */
  cfg::Port* getPort(const std::string& portName) const;

  /**
   * Get a pointer to the Interface object for a given interface ID.
   *
   * @param interfaceId The interface ID
   * @return Pointer to the Interface object if found, nullptr otherwise
   */
  cfg::Interface* getInterface(int32_t interfaceId) const;

  /**
   * Get a pointer to the Interface object for a given interface name.
   *
   * @param interfaceName The interface name
   * @return Pointer to the Interface object if found, nullptr otherwise
   */
  cfg::Interface* getInterfaceByName(const std::string& interfaceName) const;

 private:
  // Map from port name to port logical ID
  std::unordered_map<std::string, int32_t> portNameToLogicalId_;

  // Map from port logical ID to port name
  std::unordered_map<int32_t, std::string> portLogicalIdToName_;

  // Map from port logical ID to VLAN ID (from vlanPorts)
  std::unordered_map<int32_t, int32_t> portLogicalIdToVlanId_;

  // Map from VLAN ID to port logical ID (from vlanPorts)
  std::unordered_map<int32_t, int32_t> vlanIdToPortLogicalId_;

  // Map from interface ID to VLAN ID
  std::unordered_map<int32_t, int32_t> interfaceIdToVlanId_;

  // Map from VLAN ID to interface ID
  std::unordered_map<int32_t, int32_t> vlanIdToInterfaceId_;

  // Map from port name to interface ID (the final mapping)
  std::unordered_map<std::string, int32_t> portNameToInterfaceId_;

  // Map from interface ID to port name (reverse mapping)
  std::unordered_map<int32_t, std::string> interfaceIdToPortName_;

  // Map from port name to Port object pointer
  std::unordered_map<std::string, cfg::Port*> portNameToPort_;

  // Map from interface ID to Interface object pointer
  std::unordered_map<int32_t, cfg::Interface*> interfaceIdToInterface_;

  // Map from interface name to Interface object pointer
  std::unordered_map<std::string, cfg::Interface*> interfaceNameToInterface_;

  /**
   * Build the port name to logical ID mapping.
   */
  void buildPortMaps(const cfg::SwitchConfig& switchConfig);

  /**
   * Build the VLAN-based mappings.
   */
  void buildVlanMaps(const cfg::SwitchConfig& switchConfig);

  /**
   * Build the interface mappings.
   */
  void buildInterfaceMaps(const cfg::SwitchConfig& switchConfig);

  /**
   * Map an interface to a port using various strategies.
   */
  void mapInterfaceToPort(const cfg::Interface& interface);
};

} // namespace facebook::fboss::utils
