// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <vector>

#include <folly/IPAddress.h>
#include <folly/String.h>

namespace facebook::fboss::platform::helpers {

// Parse a comma-separated list of CIDR subnets
// (e.g. "10.0.0.0/8,2401:db00::/32") into folly::CIDRNetwork. Empty and
// whitespace-only entries are skipped. Throws folly::IPAddressFormatException
// on a malformed subnet so a misconfigured allowlist fails loudly rather than
// silently admitting nobody.
inline std::vector<folly::CIDRNetwork> parseTrustedSubnets(
    const std::string& csv) {
  std::vector<folly::CIDRNetwork> subnets;
  std::vector<folly::StringPiece> tokens;
  folly::split(',', csv, tokens);
  for (const auto& token : tokens) {
    const auto trimmed = folly::trimWhitespace(token);
    if (trimmed.empty()) {
      continue;
    }
    subnets.push_back(folly::IPAddress::createNetwork(trimmed));
  }
  return subnets;
}

// A client is trusted iff it connects from loopback or from one of the
// configured trusted subnets. Loopback is always trusted because the platform
// services talk to each other over localhost.
inline bool isTrustedClient(
    const folly::IPAddress& clientIp,
    const std::vector<folly::CIDRNetwork>& trustedSubnets) {
  if (clientIp.isLoopback()) {
    return true;
  }
  for (const auto& subnet : trustedSubnets) {
    if (clientIp.inSubnet(subnet.first, subnet.second)) {
      return true;
    }
  }
  return false;
}

} // namespace facebook::fboss::platform::helpers
