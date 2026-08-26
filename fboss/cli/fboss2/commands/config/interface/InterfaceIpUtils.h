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

#include <fmt/format.h>
#include <folly/IPAddress.h>
#include <stdexcept>
#include <string>

namespace facebook::fboss {

// Parses `value` as a CIDR network and validates that its IP version matches
// `attr` ("ip-address" expects IPv4, "ipv6-address" expects IPv6).
// Throws folly::IPAddressFormatException on bad CIDR, std::invalid_argument on
// version mismatch. Callers should let these escape to the CLI error handler.
inline std::pair<folly::IPAddress, uint8_t> validateInterfaceIpAttr(
    const std::string& attr,
    const std::string& value) {
  auto result = folly::IPAddress::createNetwork(value);
  bool expectV6 = (attr == "ipv6-address");
  if (expectV6 && !result.first.isV6()) {
    throw std::invalid_argument(
        fmt::format(
            "Expected IPv6 address for 'ipv6-address', got IPv4: {}", value));
  }
  if (!expectV6 && result.first.isV6()) {
    throw std::invalid_argument(
        fmt::format(
            "Expected IPv4 address for 'ip-address', got IPv6: {}", value));
  }
  return result;
}

} // namespace facebook::fboss
