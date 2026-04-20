/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/acl/table/CmdConfigAclTable.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/config/acl/AclConfigUtils.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {
constexpr std::string_view kAclTableAttrPriority = "priority";
} // namespace

AclTableConfigArgs::AclTableConfigArgs(std::vector<std::string> v) {
  if (v.size() != 4) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <group-name> <table-name> priority <value>, got {} argument(s)",
            v.size()));
  }
  if (v[2] != kAclTableAttrPriority) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown attribute '{}' for acl table. Valid attrs: priority",
            v[2]));
  }

  int16_t parsed = 0;
  try {
    parsed = folly::to<int16_t>(v[3]);
  } catch (const folly::ConversionError&) {
    throw std::invalid_argument(
        fmt::format("Value for 'priority' must be an integer, got '{}'", v[3]));
  }
  if (parsed < 0) {
    throw std::invalid_argument(
        fmt::format(
            "Value for 'priority' must be non-negative, got {}", parsed));
  }

  groupName_ = v[0];
  tableName_ = v[1];
  priority_ = parsed;
  data_ = std::move(v);
}

CmdConfigAclTableTraits::RetType CmdConfigAclTable::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  auto& group =
      acl_utils::findAclTableGroupOrThrow(swConfig, args.getGroupName());

  auto& tables = *group.aclTables();
  auto tableIt =
      std::find_if(tables.begin(), tables.end(), [&](const cfg::AclTable& t) {
        return *t.name() == args.getTableName();
      });
  if (tableIt == tables.end()) {
    throw std::runtime_error(
        fmt::format(
            "AclTable '{}' not found in group '{}'",
            args.getTableName(),
            args.getGroupName()));
  }

  tableIt->priority() = args.getPriority();

  // AclTable priority changes are applied at runtime via
  // processAclTableGroupDelta (SaiAclTableManager); no change-prohibited guard
  // exists in SaiSwitch.
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  return fmt::format(
      "Set acl table '{}' (group '{}') priority to {}",
      args.getTableName(),
      args.getGroupName(),
      args.getPriority());
}

void CmdConfigAclTable::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdConfigAclTable, CmdConfigAclTableTraits>::run();

} // namespace facebook::fboss
