/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/sflow/CmdConfigInterfaceSflow.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

namespace {
constexpr std::string_view kAttrSampleDest = "sample-dest";
constexpr std::string_view kSampleDestCpu = "cpu";
constexpr std::string_view kSampleDestMirror = "mirror";
constexpr auto kValidSflowAttrs = "sample-dest";

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return s;
}

cfg::SampleDestination parseSampleDest(const std::string& token) {
  if (token == kSampleDestCpu) {
    return cfg::SampleDestination::CPU;
  }
  if (token == kSampleDestMirror) {
    return cfg::SampleDestination::MIRROR;
  }
  throw std::invalid_argument(
      fmt::format(
          "Invalid sample destination '{}': must be {} or {}",
          token,
          kSampleDestCpu,
          kSampleDestMirror));
}
} // namespace

SflowAttrArgs::SflowAttrArgs(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument(
        fmt::format(
            "No sflow attribute provided. Valid attributes are: {}",
            kValidSflowAttrs));
  }
  attr_ = toLower(v[0]);
  if (v.size() != 2) {
    throw std::invalid_argument(
        fmt::format("Expected exactly one value for '{}'", attr_));
  }
  value_ = v[1];
  data_ = std::move(v);
}

CmdConfigInterfaceSflowTraits::RetType CmdConfigInterfaceSflow::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& sflowAttr) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  if (sflowAttr.attr() != kAttrSampleDest) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown sflow attribute '{}'. Valid attributes are: {}",
            sflowAttr.attr(),
            kValidSflowAttrs));
  }

  std::string token = toLower(sflowAttr.value());
  cfg::SampleDestination dest = parseSampleDest(token);

  auto& session = ConfigSession::getInstance();

  std::vector<std::string> updatedNames;
  std::vector<std::string> skippedNames;
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      // Resolved as an L3 interface only (e.g. an SVI): sampleDest is a Port
      // attribute, so there is nothing to set — report it rather than
      // silently succeeding.
      skippedNames.push_back(intf.name());
      continue;
    }
    // The agent rejects egress sampling to a mirror destination
    // (ApplyThriftConfig throws for MIRROR + sFlowEgressRate > 0); fail here
    // with a targeted message before touching the config.
    if (dest == cfg::SampleDestination::MIRROR &&
        *port->sFlowEgressRate() > 0) {
      throw std::invalid_argument(
          fmt::format(
              "Port {}: sample-dest {} requires sFlowEgressRate 0 — egress "
              "sampling to a mirror destination is unsupported",
              *port->name(),
              kSampleDestMirror));
    }
    port->sampleDest() = dest;
    updatedNames.push_back(intf.name());
  }
  if (updatedNames.empty()) {
    throw std::invalid_argument("No port found for the specified interface(s)");
  }

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  std::string message = fmt::format(
      "Successfully set sFlow sample destination for interface(s) {} to {}",
      folly::join(", ", updatedNames),
      token);
  if (!skippedNames.empty()) {
    message +=
        fmt::format("; skipped (no port): {}", folly::join(", ", skippedNames));
  }
  return message;
}

void CmdConfigInterfaceSflow::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigInterfaceSflow, CmdConfigInterfaceSflowTraits>::run();

} // namespace facebook::fboss
