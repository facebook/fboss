/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/acl/rule/CmdDeleteAclRule.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/config/acl/rule/AclRuleAttrs.h"

#include <fmt/format.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

AclRuleDeleteArgs::AclRuleDeleteArgs(std::vector<std::string> v) {
  if (v.size() != 2) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <table-name> <rule-name>, got {} argument(s)", v.size()));
  }
  tableName_ = v[0];
  ruleName_ = v[1];
  data_ = std::move(v);
}

CmdDeleteAclRuleTraits::RetType CmdDeleteAclRule::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  auto [matchingTable, matchingGroupName] =
      findAclTable(swConfig, args.getTableName());

  auto& entries = *matchingTable->aclEntries();
  auto eit =
      std::find_if(entries.begin(), entries.end(), [&](const cfg::AclEntry& e) {
        return *e.name() == args.getRuleName();
      });
  if (eit == entries.end()) {
    throw std::runtime_error(
        fmt::format(
            "AclEntry '{}' not found in table '{}' (group '{}')",
            args.getRuleName(),
            args.getTableName(),
            matchingGroupName));
  }

  entries.erase(eit);

  // Drop any MatchToAction attached to this rule. ApplyThriftConfig's
  // checkTrafficPolicyAclsExistInConfig rejects a config where a matcher
  // references a non-existent AclEntry, so a leftover matchToAction would
  // make `config session commit` fail. It would also silently re-attach
  // on rule re-creation under the same name.
  if (auto policy = swConfig.dataPlaneTrafficPolicy()) {
    auto& mtaList = *policy->matchToAction();
    mtaList.erase(
        std::remove_if(
            mtaList.begin(),
            mtaList.end(),
            [&](const cfg::MatchToAction& mta) {
              return *mta.matcher() == args.getRuleName();
            }),
        mtaList.end());
  }

  // Removing an AclEntry takes the same dataplane path as mutating one —
  // processAclTableGroupDelta in SaiAclTableManager applies the diff at
  // runtime. SaiSwitch has no warmboot-prohibited guard for ACL entry
  // removal, so this is HITLESS.
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  return fmt::format(
      "Deleted acl rule '{}' from table '{}' (group '{}')",
      args.getRuleName(),
      args.getTableName(),
      matchingGroupName);
}

void CmdDeleteAclRule::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

template void CmdHandler<CmdDeleteAclRule, CmdDeleteAclRuleTraits>::run();

} // namespace facebook::fboss
