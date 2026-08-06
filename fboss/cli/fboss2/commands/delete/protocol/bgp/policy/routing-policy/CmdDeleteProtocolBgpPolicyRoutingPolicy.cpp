/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/routing-policy/CmdDeleteProtocolBgpPolicyRoutingPolicy.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <algorithm>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {
constexpr std::string_view kObjectName = "routing-policy";
} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
BgpRoutingPolicyRef::BgpRoutingPolicyRef(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: delete protocol bgp policy routing-policy requires <name>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument(
        fmt::format("Error: {} name must not be empty", kObjectName));
  }
  // Unlike the config grammar, nothing may follow the name: there are no
  // attributes to delete through this command.
  if (v.size() > 1) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unexpected token '{}'. Usage: delete protocol bgp "
            "policy routing-policy <name>",
            v[1]));
  }
  policyName_ = v[0];
}

CmdDeleteProtocolBgpPolicyRoutingPolicyTraits::RetType
CmdDeleteProtocolBgpPolicyRoutingPolicy::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  if (!cfg.policies().has_value()) {
    // Nothing is persisted for an unknown policy, so a typo'd delete can't
    // stage an unrelated session change.
    return fmt::format(
        "Error: BGP routing-policy {} not found", args.policyName());
  }
  auto& policies = *cfg.policies()->bgp_policy_statements();
  auto it =
      std::find_if(policies.begin(), policies.end(), [&](const auto& policy) {
        return *policy.name() == args.policyName();
      });
  if (it == policies.end()) {
    return fmt::format(
        "Error: BGP routing-policy {} not found", args.policyName());
  }
  policies.erase(it);
  session.saveBgpConfig();
  return fmt::format(
      "Successfully deleted BGP routing-policy {}\nConfig saved to: {}",
      args.policyName(),
      session.getBgpSessionConfigPath());
}

void CmdDeleteProtocolBgpPolicyRoutingPolicy::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdDeleteProtocolBgpPolicyRoutingPolicy,
    CmdDeleteProtocolBgpPolicyRoutingPolicyTraits>::run();

} // namespace facebook::fboss
