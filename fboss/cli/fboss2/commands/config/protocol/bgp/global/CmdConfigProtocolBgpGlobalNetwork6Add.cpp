/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalNetwork6Add.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalNetwork6AddTraits::RetType
CmdConfigProtocolBgpGlobalNetwork6Add::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = BgpConfigSession::getInstance();

  // args.data() returns a vector of strings
  // Command: "config protocol bgp global network6 add <prefix> [policy <name>]
  // [install-to-fib <bool>] [min-routes <int>]"
  // After "network6", args contains: ["add", "<prefix>", ...]
  // So we need to skip tokens[0] which is "add"
  const auto& tokens = args.data();

  if (tokens.size() < 2) {
    return "Error: prefix is required. Usage: network6 add <prefix> [policy "
           "<name>] [install-to-fib <bool>] [min-routes <int>]";
  }

  // tokens[0] is "add", tokens[1] is the actual prefix
  std::string prefixStr = tokens[1];
  std::string policyName;
  bool installToFib = false;
  int32_t minSupportingRoutes = 0;

  // Start parsing optional args from index 2
  for (size_t i = 2; i < tokens.size(); i++) {
    if (tokens[i] == "policy" && i + 1 < tokens.size()) {
      policyName = tokens[++i];
    } else if (tokens[i] == "install-to-fib" && i + 1 < tokens.size()) {
      std::string val = tokens[++i];
      installToFib = (val == "true" || val == "1");
    } else if (tokens[i] == "min-routes" && i + 1 < tokens.size()) {
      std::string minRoutesStr = tokens[++i];
      try {
        minSupportingRoutes = folly::to<int32_t>(minRoutesStr);
      } catch (const std::exception& e) {
        return fmt::format(
            "Error: Invalid min-routes value '{}': {}", minRoutesStr, e.what());
      }
    }
  }

  session.addNetwork6(prefixStr, policyName, installToFib, minSupportingRoutes);
  session.saveConfig();

  return fmt::format(
      "Successfully added network6 prefix: {} (policy: {}, install_to_fib: {}, min_routes: {})\nConfig saved to: {}",
      prefixStr,
      policyName.empty() ? "(none)" : policyName,
      installToFib ? "true" : "false",
      minSupportingRoutes,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalNetwork6Add::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
