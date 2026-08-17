/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/interface/sflow/sample_dest/CmdDeleteInterfaceSflowSampleDest.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <iostream>
#include <vector>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdDeleteInterfaceSflowSampleDestTraits::RetType
CmdDeleteInterfaceSflowSampleDest::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  auto& session = ConfigSession::getInstance();

  std::vector<std::string> updatedNames;
  std::vector<std::string> skippedNames;
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      // Resolved as an L3 interface only (e.g. an SVI): sampleDest is a Port
      // attribute, so there is nothing to clear — report it rather than
      // silently succeeding.
      skippedNames.push_back(intf.name());
      continue;
    }
    port->sampleDest().reset();
    updatedNames.push_back(intf.name());
  }
  if (updatedNames.empty()) {
    throw std::invalid_argument("No port found for the specified interface(s)");
  }

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  std::string message = fmt::format(
      "Reset sFlow sample destination for interface(s) {}",
      folly::join(", ", updatedNames));
  if (!skippedNames.empty()) {
    message +=
        fmt::format("; skipped (no port): {}", folly::join(", ", skippedNames));
  }
  return message;
}

void CmdDeleteInterfaceSflowSampleDest::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdDeleteInterfaceSflowSampleDest,
    CmdDeleteInterfaceSflowSampleDestTraits>::run();

} // namespace facebook::fboss
