/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/srv6/utils/Srv6MySidCliUtils.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/String.h>

namespace facebook::fboss {

namespace {

constexpr uint8_t kSupportedLocatorPrefixLength{32};

void expectKeyword(
    const std::vector<std::string>& tokens,
    size_t& index,
    const std::string& keyword) {
  if (index >= tokens.size() || tokens[index] != keyword) {
    throw std::invalid_argument(
        fmt::format(
            "Expected '{}', got '{}'",
            keyword,
            index < tokens.size() ? tokens[index] : "<missing>"));
  }
  index++;
}

} // namespace

std::string canonicalLocatorPrefix(const std::string& prefix) {
  if (prefix.empty()) {
    throw std::invalid_argument("Locator prefix is required");
  }

  folly::CIDRNetwork network;
  try {
    network = folly::IPAddress::createNetwork(prefix);
  } catch (const std::exception& e) {
    throw std::invalid_argument(
        fmt::format("Invalid locator prefix '{}': {}", prefix, e.what()));
  }

  if (!network.first.isV6()) {
    throw std::invalid_argument(
        fmt::format("Locator prefix must be IPv6, got '{}'", prefix));
  }

  if (network.second != kSupportedLocatorPrefixLength) {
    throw std::invalid_argument(
        fmt::format(
            "SRv6 locator prefix length must be /{}, got /{} in '{}'",
            static_cast<int>(kSupportedLocatorPrefixLength),
            static_cast<int>(network.second),
            prefix));
  }

  return folly::IPAddress::networkToString(
      std::make_pair(network.first, static_cast<uint8_t>(network.second)));
}

int16_t parseMySidFunctionValue(const std::string& value) {
  int32_t parsed = 0;
  try {
    parsed = folly::to<int32_t>(value);
  } catch (const folly::ConversionError&) {
    throw std::invalid_argument(
        fmt::format(
            "Invalid MySID function value '{}': must be an integer", value));
  }

  if (parsed < kMySidFunctionMin || parsed > kMySidFunctionMax) {
    throw std::invalid_argument(
        fmt::format(
            "MySID function value must be in range [{}, {}], got {}",
            kMySidFunctionMin,
            kMySidFunctionMax,
            value));
  }

  return static_cast<int16_t>(parsed);
}

bool parseMySidIsV6(const std::string& value) {
  std::string lowered = value;
  folly::toLowerAscii(lowered);
  if (lowered == "true") {
    return true;
  }
  if (lowered == "false") {
    return false;
  }
  throw std::invalid_argument(
      fmt::format("Invalid is-v6 value '{}': must be true or false", value));
}

bool hasMySidConfig(const cfg::SwitchConfig& swConfig) {
  return swConfig.mySidConfig().has_value();
}

cfg::MySidConfig& requireMySidConfig(cfg::SwitchConfig& swConfig) {
  if (!hasMySidConfig(swConfig)) {
    throw std::runtime_error(
        "No SRv6 MySID configured. Run: config srv6 my-sid <prefix> first.");
  }
  return *swConfig.mySidConfig();
}

std::map<int16_t, cfg::MySidEntryConfig>& ensureMySidEntries(
    cfg::MySidConfig& config) {
  if (!config.entries().has_value()) {
    config.entries() = {};
  }
  return *config.entries();
}

void requireMatchingLocatorPrefix(
    const cfg::MySidConfig& config,
    const std::string& requestedPrefix) {
  auto configured = canonicalLocatorPrefix(*config.locatorPrefix());
  auto requested = canonicalLocatorPrefix(requestedPrefix);
  if (configured != requested) {
    throw std::runtime_error(
        fmt::format(
            "Locator {} does not match configured {}", requested, configured));
  }
}

LocatorPrefixArg::LocatorPrefixArg(std::vector<std::string> v) {
  if (v.size() != 1) {
    throw std::invalid_argument(
        fmt::format(
            "Expected exactly one locator prefix argument, got {}", v.size()));
  }
  prefix_ = canonicalLocatorPrefix(v[0]);
  data_.push_back(prefix_);
}

std::string MySidAddArg::getTypeStr() const {
  switch (type_) {
    case MySidConfigEntryType::ADJACENCY:
      return "adjacency";
    case MySidConfigEntryType::NODE:
      return "node";
    case MySidConfigEntryType::DECAP:
      return "decap";
  }
  return "unknown";
}

MySidAddArg::MySidAddArg(std::vector<std::string> v) {
  size_t index = 0;
  expectKeyword(v, index, "entry");
  if (index >= v.size()) {
    throw std::invalid_argument("Missing function value after 'entry'");
  }
  functionValue_ = parseMySidFunctionValue(v[index++]);

  expectKeyword(v, index, "type");
  if (index >= v.size()) {
    throw std::invalid_argument("Missing type value after 'type'");
  }

  const auto& typeToken = v[index++];
  if (typeToken == "adjacency") {
    type_ = MySidConfigEntryType::ADJACENCY;
    expectKeyword(v, index, "is-v6");
    if (index >= v.size()) {
      throw std::invalid_argument("Missing value after 'is-v6'");
    }
    isV6_ = parseMySidIsV6(v[index++]);
    expectKeyword(v, index, "port-name");
    if (index >= v.size() || v[index].empty()) {
      throw std::invalid_argument("Missing port-name value");
    }
    portName_ = v[index++];
  } else if (typeToken == "node") {
    type_ = MySidConfigEntryType::NODE;
    expectKeyword(v, index, "node-address");
    if (index >= v.size()) {
      throw std::invalid_argument("Missing value after 'node-address'");
    }
    try {
      auto addr = folly::IPAddress(v[index]);
      if (!addr.isV6()) {
        throw std::invalid_argument(
            fmt::format("node-address must be IPv6, got '{}'", v[index]));
      }
      nodeAddress_ = addr.str();
    } catch (const std::exception& e) {
      throw std::invalid_argument(
          fmt::format("Invalid node-address '{}': {}", v[index], e.what()));
    }
    index++;
  } else if (typeToken == "decap") {
    type_ = MySidConfigEntryType::DECAP;
  } else {
    throw std::invalid_argument(
        fmt::format("Unknown MySID type '{}'", typeToken));
  }

  if (index != v.size()) {
    throw std::invalid_argument(
        fmt::format(
            "Unexpected extra argument '{}' for type {}", v[index], typeToken));
  }

  data_.push_back(fmt::format("{}", functionValue_));
}

cfg::MySidEntryConfig MySidAddArg::buildEntryConfig() const {
  cfg::MySidEntryConfig entry;
  switch (type_) {
    case MySidConfigEntryType::ADJACENCY: {
      cfg::AdjacencyMySidConfig adj;
      adj.isV6() = isV6_;
      adj.portName() = portName_;
      entry.adjacency() = adj;
      break;
    }
    case MySidConfigEntryType::NODE: {
      cfg::NodeMySidConfig node;
      node.nodeAddress() = nodeAddress_;
      entry.node() = node;
      break;
    }
    case MySidConfigEntryType::DECAP:
      entry.decap() = cfg::DecapMySidConfig{};
      break;
  }
  return entry;
}

MySidDeleteEntryArg::MySidDeleteEntryArg(std::vector<std::string> v) {
  size_t index = 0;
  expectKeyword(v, index, "entry");
  if (index >= v.size()) {
    throw std::invalid_argument("Missing function value after 'entry'");
  }
  functionValue_ = parseMySidFunctionValue(v[index++]);
  if (index != v.size()) {
    throw std::invalid_argument(
        fmt::format(
            "Unexpected extra argument '{}' for delete entry", v[index]));
  }
  data_.push_back(fmt::format("{}", functionValue_));
}

} // namespace facebook::fboss
