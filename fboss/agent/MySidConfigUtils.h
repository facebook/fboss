// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/IPAddressV6.h>
#include <string>
#include <unordered_map>

#include "fboss/agent/types.h"

namespace facebook::fboss {

// Build the full SID IPv6 address from locator address + function ID.
// locatorAddr: the locator block address (e.g., from "3001:db8::/32")
// locatorPrefixLen: the locator block length in bits (e.g., 32)
// functionId: hex string (e.g., "ffff", "1", "7fff")
// Returns the combined address (e.g., "3001:db8:ffff::")
folly::IPAddressV6 buildSidAddress(
    const folly::IPAddressV6& locatorAddr,
    uint8_t locatorPrefixLen,
    const std::string& functionId);

// Build a map from port name to InterfaceID for all physical and aggregate
// ports in the switch config.
// Physical ports: port.name → InterfaceID(port.ingressVlan), defaulting to
// "port-<logicalID>" when name is unset.
// Aggregate ports: aggPort.name → InterfaceID(aggPort.key)
std::unordered_map<std::string, InterfaceID> buildPortNameToInterfaceIdMap(
    const cfg::SwitchConfig& config);

} // namespace facebook::fboss
