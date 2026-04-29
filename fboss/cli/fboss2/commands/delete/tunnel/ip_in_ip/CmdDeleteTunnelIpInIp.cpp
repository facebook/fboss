/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/tunnel/ip_in_ip/CmdDeleteTunnelIpInIp.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

namespace {

// Optional fields that can be reset individually.
const std::unordered_set<std::string> kResettableAttrs = {
    "source",
    "ttl-mode",
    "dscp-mode",
    "ecn-mode",
    "termination-type",
};

// Display order for kResettableAttrs — used in error messages so they print
// deterministically across stdlib implementations.
const std::vector<std::string> kResettableAttrsDisplay = {
    "source",
    "ttl-mode",
    "dscp-mode",
    "ecn-mode",
    "termination-type",
};

// Required fields — cannot be reset individually; delete the whole tunnel.
const std::unordered_set<std::string> kRequiredAttrs = {
    "destination",
    "underlay-intf-id",
};

std::string toLower(const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

} // namespace

// ---------------------------------------------------------------------------
// TunnelIpInIpDeleteArgs constructor
// ---------------------------------------------------------------------------

TunnelIpInIpDeleteArgs::TunnelIpInIpDeleteArgs(
    const std::vector<std::string>& v) {
  if (v.empty()) {
    throw std::invalid_argument("Tunnel ID is required");
  }

  // First token is the tunnel ID; reject if it looks like an attribute name
  // to catch the common mistake of forgetting the ID.
  const std::string firstLower = toLower(v[0]);
  if (kResettableAttrs.count(firstLower) > 0 ||
      kRequiredAttrs.count(firstLower) > 0) {
    throw std::invalid_argument(
        fmt::format("Expected tunnel ID, got attribute name '{}'", v[0]));
  }
  tunnelId_ = v[0];
  data_ = v;

  for (size_t i = 1; i < v.size(); ++i) {
    const std::string attrLower = toLower(v[i]);

    if (kRequiredAttrs.count(attrLower) > 0) {
      throw std::invalid_argument(
          fmt::format(
              "Cannot reset required field '{}'. To remove the tunnel, run "
              "'delete tunnel ip-in-ip <tunnel-id>' with no attributes.",
              v[i]));
    }

    if (kResettableAttrs.count(attrLower) == 0) {
      throw std::invalid_argument(
          fmt::format(
              "Unknown tunnel attribute '{}'. Valid attributes: {}",
              v[i],
              folly::join(", ", kResettableAttrsDisplay)));
    }

    attributes_.push_back(attrLower);
  }
}

// ---------------------------------------------------------------------------
// CmdDeleteTunnelIpInIp::queryClient
// ---------------------------------------------------------------------------

CmdDeleteTunnelIpInIpTraits::RetType CmdDeleteTunnelIpInIp::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& deleteArgs) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  const std::string& tunnelId = deleteArgs.getTunnelId();

  if (!swConfig.ipInIpTunnels().has_value() ||
      swConfig.ipInIpTunnels()->empty()) {
    // Idempotent: nothing to delete
    return fmt::format("No IP-in-IP tunnels configured; nothing to delete");
  }

  auto& tunnels = *swConfig.ipInIpTunnels();
  auto it = std::find_if(tunnels.begin(), tunnels.end(), [&](const auto& t) {
    return *t.ipInIpTunnelId() == tunnelId;
  });
  if (it == tunnels.end()) {
    // Idempotent: tunnel doesn't exist
    return fmt::format("Tunnel '{}' not found; nothing to delete", tunnelId);
  }

  if (deleteArgs.deleteEntireTunnel()) {
    tunnels.erase(it);
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    return fmt::format("Successfully deleted tunnel '{}'", tunnelId);
  }

  // Reset the listed optional attributes.
  for (const auto& attr : deleteArgs.getAttributes()) {
    if (attr == "source") {
      it->srcIp().reset();
      it->srcIpMask().reset();
    } else if (attr == "ttl-mode") {
      it->ttlMode().reset();
    } else if (attr == "dscp-mode") {
      it->dscpMode().reset();
    } else if (attr == "ecn-mode") {
      it->ecnMode().reset();
    } else if (attr == "termination-type") {
      it->tunnelTermType().reset();
    }
  }

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  return fmt::format(
      "Successfully reset tunnel '{}': {}",
      tunnelId,
      folly::join(", ", deleteArgs.getAttributes()));
}

void CmdDeleteTunnelIpInIp::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdDeleteTunnelIpInIp, CmdDeleteTunnelIpInIpTraits>::run();

} // namespace facebook::fboss
