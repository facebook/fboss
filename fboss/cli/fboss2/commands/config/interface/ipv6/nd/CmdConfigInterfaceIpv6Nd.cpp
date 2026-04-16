/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/ipv6/nd/CmdConfigInterfaceIpv6Nd.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
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
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

namespace {

// All known attribute names for `config interface <intf> ipv6 nd`
const std::unordered_set<std::string> kKnownNdAttrs = {
    "ra-interval",
    "ra-lifetime",
    "hop-limit",
    "prefix-valid-lifetime",
    "prefix-preferred-lifetime",
    "managed-config-flag",
    "other-config-flag",
    "ra-address",
};

// Attributes that consume no value token
const std::unordered_set<std::string> kValuelessNdAttrs = {
    "managed-config-flag",
    "other-config-flag",
};

bool isKnownNdAttr(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return kKnownNdAttrs.find(lower) != kKnownNdAttrs.end();
}

// Ensure the optional NDP config is present on the interface; return a ref.
cfg::NdpConfig& ensureNdp(cfg::Interface& iface) {
  if (!iface.ndp().has_value()) {
    iface.ndp() = cfg::NdpConfig{};
  }
  return *iface.ndp();
}

} // namespace

// ---------------------------------------------------------------------------
// NdpConfigAttrs constructor — parses the token list into attr/value pairs
// ---------------------------------------------------------------------------

NdpConfigAttrs::NdpConfigAttrs(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "No NDP attribute provided. Valid attributes: "
        "ra-interval, ra-lifetime, hop-limit, prefix-valid-lifetime, "
        "prefix-preferred-lifetime, managed-config-flag, other-config-flag, "
        "ra-address");
  }

  for (size_t i = 0; i < v.size();) {
    std::string attrLower = v[i];
    std::transform(
        attrLower.begin(), attrLower.end(), attrLower.begin(), ::tolower);

    if (!isKnownNdAttr(attrLower)) {
      throw std::invalid_argument(
          fmt::format(
              "Unknown nd attribute '{}'. Valid attributes: {}",
              v[i],
              folly::join(", ", kKnownNdAttrs)));
    }

    if (kValuelessNdAttrs.count(attrLower)) {
      attributes_.emplace_back(attrLower, "");
      ++i;
      continue;
    }

    if (i + 1 >= v.size()) {
      throw std::invalid_argument(
          fmt::format("Missing value for nd attribute '{}'", v[i]));
    }

    const std::string& value = v[i + 1];

    if (isKnownNdAttr(value)) {
      throw std::invalid_argument(
          fmt::format(
              "Missing value for nd attribute '{}'. Got another attribute '{}' instead.",
              v[i],
              value));
    }

    attributes_.emplace_back(attrLower, value);
    i += 2;
  }
}

// ---------------------------------------------------------------------------
// CmdConfigInterfaceIpv6Nd::queryClient
// ---------------------------------------------------------------------------

CmdConfigInterfaceIpv6NdTraits::RetType CmdConfigInterfaceIpv6Nd::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& ndpAttrs) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  const auto& attributes = ndpAttrs.getAttributes();
  if (attributes.empty()) {
    throw std::invalid_argument(
        "No NDP attribute provided. Valid attributes: "
        "ra-interval, ra-lifetime, hop-limit, prefix-valid-lifetime, "
        "prefix-preferred-lifetime, managed-config-flag, other-config-flag, "
        "ra-address");
  }

  std::vector<std::string> results;

  for (const auto& [attr, value] : attributes) {
    if (attr == "ra-interval") {
      int32_t seconds = 0;
      try {
        seconds = folly::to<int32_t>(value);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            fmt::format(
                "Invalid ra-interval value '{}': must be a non-negative integer",
                value));
      }
      if (seconds < 0) {
        throw std::invalid_argument(
            fmt::format(
                "ra-interval value {} is out of range. Must be >= 0", seconds));
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).routerAdvertisementSeconds() = seconds;
        }
      }
      results.push_back(fmt::format("ra-interval={}", seconds));

    } else if (attr == "ra-lifetime") {
      int32_t seconds = 0;
      try {
        seconds = folly::to<int32_t>(value);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            fmt::format(
                "Invalid ra-lifetime value '{}': must be a non-negative integer",
                value));
      }
      if (seconds < 0) {
        throw std::invalid_argument(
            fmt::format(
                "ra-lifetime value {} is out of range. Must be >= 0", seconds));
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).routerLifetime() = seconds;
        }
      }
      results.push_back(fmt::format("ra-lifetime={}", seconds));

    } else if (attr == "hop-limit") {
      int32_t hopLimit = 0;
      try {
        hopLimit = folly::to<int32_t>(value);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            fmt::format(
                "Invalid hop-limit value '{}': must be an integer", value));
      }
      if (hopLimit < 0 || hopLimit > 255) {
        throw std::invalid_argument(
            fmt::format(
                "hop-limit value {} is out of range. Valid range: 0-255",
                hopLimit));
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).curHopLimit() = hopLimit;
        }
      }
      results.push_back(fmt::format("hop-limit={}", hopLimit));

    } else if (attr == "prefix-valid-lifetime") {
      int32_t seconds = 0;
      try {
        seconds = folly::to<int32_t>(value);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            fmt::format(
                "Invalid prefix-valid-lifetime value '{}': must be a non-negative integer",
                value));
      }
      if (seconds < 0) {
        throw std::invalid_argument(
            fmt::format(
                "prefix-valid-lifetime value {} is out of range. Valid range: >= 0",
                seconds));
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).prefixValidLifetimeSeconds() = seconds;
        }
      }
      results.push_back(fmt::format("prefix-valid-lifetime={}", seconds));

    } else if (attr == "prefix-preferred-lifetime") {
      int32_t seconds = 0;
      try {
        seconds = folly::to<int32_t>(value);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            fmt::format(
                "Invalid prefix-preferred-lifetime value '{}': must be a non-negative integer",
                value));
      }
      if (seconds < 0) {
        throw std::invalid_argument(
            fmt::format(
                "prefix-preferred-lifetime value {} is out of range. Valid range: >= 0",
                seconds));
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).prefixPreferredLifetimeSeconds() = seconds;
        }
      }
      results.push_back(fmt::format("prefix-preferred-lifetime={}", seconds));

    } else if (attr == "managed-config-flag") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).routerAdvertisementManagedBit() = true;
        }
      }
      results.emplace_back("managed-config-flag=true");

    } else if (attr == "other-config-flag") {
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).routerAdvertisementOtherBit() = true;
        }
      }
      results.emplace_back("other-config-flag=true");

    } else if (attr == "ra-address") {
      if (value.empty()) {
        throw std::invalid_argument("ra-address cannot be empty");
      }
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (iface) {
          ensureNdp(*iface).routerAddress() = value;
        }
      }
      results.push_back(fmt::format("ra-address={}", value));
    }
  }

  ConfigSession::getInstance().saveConfig();

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  std::string attrList = folly::join(", ", results);
  return fmt::format(
      "Successfully configured interface(s) {}: {}", interfaceList, attrList);
}

void CmdConfigInterfaceIpv6Nd::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigInterfaceIpv6Nd, CmdConfigInterfaceIpv6NdTraits>::run();

} // namespace facebook::fboss
