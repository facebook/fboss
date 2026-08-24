/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/interface/sflow/CmdDeleteInterfaceSflow.h"

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
constexpr std::string_view kAttrIngressRate = "ingress-rate";
constexpr std::string_view kAttrEgressRate = "egress-rate";
constexpr auto kValidSflowAttrs = "sample-dest, ingress-rate, egress-rate";
} // namespace

SflowDeleteAttrArg::SflowDeleteAttrArg(std::vector<std::string> v) {
  if (v.size() != 1) {
    throw std::invalid_argument(
        fmt::format(
            "Expected exactly one sflow attribute to reset. Valid "
            "attributes are: {}",
            kValidSflowAttrs));
  }
  attr_ = v[0];
  std::transform(
      attr_.begin(), attr_.end(), attr_.begin(), [](unsigned char c) {
        return std::tolower(c);
      });
  data_ = std::move(v);
}

CmdDeleteInterfaceSflowTraits::RetType CmdDeleteInterfaceSflow::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& sflowAttr) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  const std::string& attr = sflowAttr.attr();
  if (attr != kAttrSampleDest && attr != kAttrIngressRate &&
      attr != kAttrEgressRate) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown sflow attribute '{}'. Valid attributes are: {}",
            attr,
            kValidSflowAttrs));
  }

  auto& session = ConfigSession::getInstance();

  std::vector<std::string> updatedNames;
  std::vector<std::string> skippedNames;
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      // Resolved as an L3 interface only (e.g. an SVI): these sflow
      // attributes are all Port attributes, so there is nothing to clear --
      // report it rather than silently succeeding.
      skippedNames.push_back(intf.name());
      continue;
    }
    if (attr == kAttrSampleDest) {
      port->sampleDest().reset();
    } else if (attr == kAttrIngressRate) {
      port->sFlowIngressRate() = 0;
    } else {
      port->sFlowEgressRate() = 0;
    }
    updatedNames.push_back(intf.name());
  }
  if (updatedNames.empty()) {
    throw std::invalid_argument("No port found for the specified interface(s)");
  }

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  std::string attrLabel = attr == kAttrSampleDest
      ? "sample destination"
      : (attr == kAttrIngressRate ? "ingress-rate" : "egress-rate");
  std::string message = fmt::format(
      "Reset sFlow {} for interface(s) {}",
      attrLabel,
      folly::join(", ", updatedNames));
  if (!skippedNames.empty()) {
    message +=
        fmt::format("; skipped (no port): {}", folly::join(", ", skippedNames));
  }
  return message;
}

void CmdDeleteInterfaceSflow::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdDeleteInterfaceSflow, CmdDeleteInterfaceSflowTraits>::run();

} // namespace facebook::fboss
