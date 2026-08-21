/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/sflow/sample_dest/CmdConfigInterfaceSflowSampleDest.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

namespace {
constexpr std::string_view kSampleDestCpu = "cpu";
constexpr std::string_view kSampleDestMirror = "mirror";
} // namespace

SampleDestValue::SampleDestValue(std::vector<std::string> v) {
  if (v.size() != 1) {
    throw std::invalid_argument(
        fmt::format(
            "Expected exactly one argument: <{}|{}>",
            kSampleDestCpu,
            kSampleDestMirror));
  }

  std::string token = v[0];
  std::transform(
      token.begin(), token.end(), token.begin(), [](unsigned char c) {
        return std::tolower(c);
      });
  if (token == kSampleDestCpu) {
    dest_ = cfg::SampleDestination::CPU;
  } else if (token == kSampleDestMirror) {
    dest_ = cfg::SampleDestination::MIRROR;
  } else {
    throw std::invalid_argument(
        fmt::format(
            "Invalid sample destination '{}': must be {} or {}",
            v[0],
            kSampleDestCpu,
            kSampleDestMirror));
  }
  token_ = std::move(token);
  data_ = std::move(v);
}

CmdConfigInterfaceSflowSampleDestTraits::RetType
CmdConfigInterfaceSflowSampleDest::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& sampleDest) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  auto& session = ConfigSession::getInstance();

  cfg::SampleDestination dest = sampleDest.getDestination();

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
      sampleDest.getToken());
  if (!skippedNames.empty()) {
    message +=
        fmt::format("; skipped (no port): {}", folly::join(", ", skippedNames));
  }
  return message;
}

void CmdConfigInterfaceSflowSampleDest::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigInterfaceSflowSampleDest,
    CmdConfigInterfaceSflowSampleDestTraits>::run();

} // namespace facebook::fboss
