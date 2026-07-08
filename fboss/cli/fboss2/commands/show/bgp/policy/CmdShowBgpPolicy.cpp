/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/policy/CmdShowBgpPolicy.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <folly/json/json.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"

namespace facebook::fboss {

CmdShowBgpPolicy::RetType CmdShowBgpPolicy::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& policyNames) {
  if (policyNames.empty()) {
    std::cerr << "No policy name provided. Usage: show bgp policy <policy_name>"
              << std::endl;
    return folly::dynamic::object;
  }

  const auto& policyName = policyNames[0];

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  facebook::bgp::thrift::BgpConfig bgpConfig;
  client->sync_getRunningConfigStruct(bgpConfig);

  auto configJson = facebook::thrift::to_dynamic(
      bgpConfig, facebook::thrift::dynamic_format::PORTABLE);

  if (!configJson.isObject() || !configJson.count("policies")) {
    std::cerr << "No policies found in config" << std::endl;
    return folly::dynamic::object;
  }

  const auto& policiesJson = configJson["policies"];
  if (!policiesJson.isObject() ||
      !policiesJson.count("bgp_policy_statements")) {
    std::cerr << "No policy statements found in config" << std::endl;
    return folly::dynamic::object;
  }

  const auto& statements = policiesJson["bgp_policy_statements"];
  for (const auto& statement : statements) {
    if (statement.isObject() && statement.count("name") &&
        statement["name"].asString() == policyName) {
      return statement;
    }
  }

  std::cerr << "Policy '" << policyName << "' not found" << std::endl;
  return folly::dynamic::object;
}

void CmdShowBgpPolicy::printOutput(RetType& policyConfig, std::ostream& out) {
  if (policyConfig.empty()) {
    return;
  }
  out << folly::toPrettyJson(policyConfig) << std::endl;
}

template void CmdHandler<CmdShowBgpPolicy, CmdShowBgpPolicyTraits>::run();

} // namespace facebook::fboss
