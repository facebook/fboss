// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MySidConfigUtils.h"

#include "fboss/agent/FbossError.h"

#include <folly/Conv.h>

namespace facebook::fboss {

folly::IPAddressV6 buildSidAddress(
    const folly::IPAddressV6& locatorAddr,
    uint8_t locatorPrefixLen,
    const std::string& functionId) {
  // Parse function ID from hex string
  uint16_t funcVal;
  try {
    funcVal = folly::to<uint16_t>(std::stoul(functionId, nullptr, 16));
  } catch (const std::exception& e) {
    throw FbossError(
        "Invalid MySID function ID '", functionId, "': ", e.what());
  }

  // The function ID occupies the 16 bits immediately after the locator block.
  // For a /32 locator, that's bits 32-47 (the 3rd group of the IPv6 address).
  // Locator prefix length must be byte-aligned (multiple of 8).
  if (locatorPrefixLen % 8 != 0) {
    throw FbossError(
        "Locator prefix length ",
        static_cast<int>(locatorPrefixLen),
        " must be a multiple of 8");
  }
  auto bytes = locatorAddr.toByteArray();
  uint8_t funcByteOffset = locatorPrefixLen / 8;
  if (funcByteOffset + 2 > bytes.size()) {
    throw FbossError(
        "Locator prefix length ",
        locatorPrefixLen,
        " too large for function ID placement");
  }
  bytes[funcByteOffset] = static_cast<uint8_t>(funcVal >> 8);
  bytes[funcByteOffset + 1] = static_cast<uint8_t>(funcVal & 0xFF);
  return folly::IPAddressV6(bytes);
}

std::unordered_map<std::string, InterfaceID> buildPortNameToInterfaceIdMap(
    const cfg::SwitchConfig& config) {
  std::unordered_map<std::string, InterfaceID> portNameToIntfId;
  for (const auto& port : *config.ports()) {
    auto name = port.name().has_value() && !port.name()->empty()
        ? *port.name()
        : folly::to<std::string>("port-", *port.logicalID());
    portNameToIntfId[name] = InterfaceID(static_cast<int>(*port.ingressVlan()));
  }
  for (const auto& aggPort : *config.aggregatePorts()) {
    portNameToIntfId[*aggPort.name()] =
        InterfaceID(static_cast<int>(*aggPort.key()));
  }
  return portNameToIntfId;
}

} // namespace facebook::fboss
