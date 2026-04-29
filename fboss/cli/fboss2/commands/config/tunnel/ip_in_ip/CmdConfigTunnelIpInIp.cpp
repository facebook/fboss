/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/tunnel/ip_in_ip/CmdConfigTunnelIpInIp.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/IPAddressV6.h>
#include <folly/String.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

namespace {

const std::unordered_set<std::string> kKnownAttrs = {
    "destination",
    "source",
    "ttl-mode",
    "dscp-mode",
    "ecn-mode",
    "termination-type",
    "underlay-intf-id",
};

// Reserved sub-keywords that follow an attribute and must never be mistaken
// for a tunnel ID (e.g. `config tunnel ip-in-ip mask 1.2.3.4` should fail).
const std::unordered_set<std::string> kReservedFirstTokens = {"mask"};

std::string toLower(const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

cfg::TunnelMode parseTunnelMode(const std::string& s) {
  const std::string lower = toLower(s);
  if (lower == kTunnelModeUniform) {
    return cfg::TunnelMode::UNIFORM;
  }
  if (lower == kTunnelModePipe) {
    return cfg::TunnelMode::PIPE;
  }
  throw std::invalid_argument(
      fmt::format("Invalid mode '{}'. Valid values: uniform, pipe", s));
}

cfg::TunnelTerminationType parseTerminationType(const std::string& s) {
  const std::string lower = toLower(s);
  if (lower == kTermTypeP2P) {
    return cfg::TunnelTerminationType::P2P;
  }
  if (lower == kTermTypeP2MP) {
    return cfg::TunnelTerminationType::P2MP;
  }
  if (lower == kTermTypeMP2P) {
    return cfg::TunnelTerminationType::MP2P;
  }
  if (lower == kTermTypeMP2MP) {
    return cfg::TunnelTerminationType::MP2MP;
  }
  throw std::invalid_argument(
      fmt::format(
          "Invalid termination-type '{}'. Valid values: p2p, p2mp, mp2p, mp2mp",
          s));
}

cfg::IpInIpTunnel& findOrCreateTunnel(
    cfg::SwitchConfig& cfg,
    const std::string& id) {
  if (!cfg.ipInIpTunnels().has_value()) {
    cfg.ipInIpTunnels() = std::vector<cfg::IpInIpTunnel>{};
  }
  auto it = std::find_if(
      cfg.ipInIpTunnels()->begin(),
      cfg.ipInIpTunnels()->end(),
      [&id](const auto& t) { return *t.ipInIpTunnelId() == id; });
  if (it != cfg.ipInIpTunnels()->end()) {
    return *it;
  }
  cfg::IpInIpTunnel t;
  t.ipInIpTunnelId() = id;
  t.tunnelType() = ::facebook::fboss::TunnelType::IP_IN_IP;
  cfg.ipInIpTunnels()->push_back(std::move(t));
  return cfg.ipInIpTunnels()->back();
}

} // namespace

bool TunnelIpInIpConfig::isKnownAttribute(const std::string& s) {
  return kKnownAttrs.find(s) != kKnownAttrs.end();
}

TunnelIpInIpConfig::TunnelIpInIpConfig(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument("Tunnel ID is required");
  }

  // Reject first-token-is-known-attr or reserved sub-keyword to catch the
  // common mistake of forgetting the tunnel ID. Match case-insensitively.
  const std::string firstLower = toLower(v[0]);
  if (isKnownAttribute(firstLower) ||
      kReservedFirstTokens.count(firstLower) > 0) {
    throw std::invalid_argument(
        fmt::format("Expected tunnel ID, got attribute name '{}'", v[0]));
  }

  tunnelId_ = v[0];
  data_ = v;

  size_t i = 1;
  while (i < v.size()) {
    const std::string attr = toLower(v[i]);
    if (!isKnownAttribute(attr)) {
      throw std::invalid_argument(
          fmt::format(
              "Unknown attribute '{}'. Valid attributes: destination, source, "
              "ttl-mode, dscp-mode, ecn-mode, termination-type, underlay-intf-id",
              v[i]));
    }
    ++i;

    if (i >= v.size() || isKnownAttribute(toLower(v[i]))) {
      throw std::invalid_argument(
          fmt::format("Missing value for attribute '{}'", attr));
    }

    // Handle destination and source: IPv6 addresses only, optional prefix-len
    if (attr == "destination" || attr == "source") {
      const std::string& ip = v[i];
      if (!folly::IPAddressV6::validate(ip)) {
        throw std::invalid_argument(
            fmt::format(
                "Invalid IPv6 address '{}' (only IPv6 is supported)", ip));
      }
      attrs_[attr] = ip;
      ++i;

      // Check for optional "mask <prefix-len>" (integer in [0, 128])
      if (i < v.size() && toLower(v[i]) == "mask") {
        ++i;
        if (i >= v.size()) {
          throw std::invalid_argument(
              fmt::format(
                  "Missing prefix length after 'mask' keyword for '{}'", attr));
        }
        const std::string& prefixLenStr = v[i];
        int prefixLen = -1;
        try {
          prefixLen = std::stoi(prefixLenStr);
        } catch (...) {
          throw std::invalid_argument(
              fmt::format(
                  "Invalid prefix length '{}': must be an integer in [0, 128]",
                  prefixLenStr));
        }
        if (prefixLen < 0 || prefixLen > 128) {
          throw std::invalid_argument(
              fmt::format("Prefix length {} out of range [0, 128]", prefixLen));
        }
        attrs_[attr + "-mask"] = prefixLenStr;
        ++i;
      }
    } else if (
        attr == "ttl-mode" || attr == "dscp-mode" || attr == "ecn-mode") {
      // parseTunnelMode validates and lowercases the value; store the
      // canonical lowercase form so queryClient's string compare succeeds
      // regardless of how the user typed it.
      attrs_[attr] = toLower(v[i]);
      parseTunnelMode(attrs_[attr]);
      ++i;
    } else if (attr == "termination-type") {
      attrs_[attr] = toLower(v[i]);
      parseTerminationType(attrs_[attr]);
      ++i;
    } else if (attr == "underlay-intf-id") {
      const std::string& intfId = v[i];
      try {
        std::stoi(intfId);
      } catch (...) {
        throw std::invalid_argument(
            fmt::format(
                "'underlay-intf-id' must be an integer, got '{}'", intfId));
      }
      attrs_[attr] = intfId;
      ++i;
    }
  }
}

CmdConfigTunnelIpInIpTraits::RetType CmdConfigTunnelIpInIp::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& tunnelConfig) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  const std::string& tunnelId = tunnelConfig.getTunnelId();
  cfg::IpInIpTunnel& tunnel = findOrCreateTunnel(swConfig, tunnelId);
  // findOrCreateTunnel sets this on creation; setting again is harmless and
  // guards against pre-existing entries with an unset/wrong type.
  tunnel.tunnelType() = ::facebook::fboss::TunnelType::IP_IN_IP;

  std::vector<std::string> results;

  for (const auto& [attr, value] : tunnelConfig.getAttrs()) {
    if (attr == "destination") {
      tunnel.dstIp() = value;
      auto maskIt = tunnelConfig.getAttrs().find("destination-mask");
      if (maskIt != tunnelConfig.getAttrs().end()) {
        tunnel.dstIpMask() = maskIt->second;
        results.push_back(
            fmt::format("destination={} prefix-len={}", value, maskIt->second));
      } else {
        results.push_back(fmt::format("destination={}", value));
      }
    } else if (attr == "source") {
      tunnel.srcIp() = value;
      auto maskIt = tunnelConfig.getAttrs().find("source-mask");
      if (maskIt != tunnelConfig.getAttrs().end()) {
        tunnel.srcIpMask() = maskIt->second;
        results.push_back(
            fmt::format("source={} prefix-len={}", value, maskIt->second));
      } else {
        results.push_back(fmt::format("source={}", value));
      }
    } else if (attr == "destination-mask" || attr == "source-mask") {
      // Skip - already handled with destination/source
    } else if (attr == "ttl-mode") {
      tunnel.ttlMode() = parseTunnelMode(value);
      results.push_back(fmt::format("ttl-mode={}", value));
    } else if (attr == "dscp-mode") {
      tunnel.dscpMode() = parseTunnelMode(value);
      results.push_back(fmt::format("dscp-mode={}", value));
    } else if (attr == "ecn-mode") {
      tunnel.ecnMode() = parseTunnelMode(value);
      results.push_back(fmt::format("ecn-mode={}", value));
    } else if (attr == "termination-type") {
      tunnel.tunnelTermType() = parseTerminationType(value);
      results.push_back(fmt::format("termination-type={}", value));
    } else if (attr == "underlay-intf-id") {
      int intfId = std::stoi(value);
      bool found = false;
      for (const auto& intf : *swConfig.interfaces()) {
        if (*intf.intfID() == intfId) {
          found = true;
          break;
        }
      }
      if (!found) {
        throw std::invalid_argument(
            fmt::format(
                "Interface ID {} not found in config. Use 'fboss2-dev show interface' to see valid IDs.",
                intfId));
      }
      tunnel.underlayIntfID() = intfId;
      results.push_back(fmt::format("underlay-intf-id={}", value));
    }
  }

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  if (results.empty()) {
    return fmt::format("Tunnel '{}' created", tunnelId);
  }
  return fmt::format(
      "Successfully configured tunnel '{}': {}",
      tunnelId,
      folly::join(", ", results));
}

void CmdConfigTunnelIpInIp::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigTunnelIpInIp, CmdConfigTunnelIpInIpTraits>::run();

} // namespace facebook::fboss
