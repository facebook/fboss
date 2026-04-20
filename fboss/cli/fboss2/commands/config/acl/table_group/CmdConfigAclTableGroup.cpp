/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/acl/table_group/CmdConfigAclTableGroup.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/config/acl/AclConfigUtils.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

namespace {
constexpr std::string_view kAclTableGroupAttrStage = "stage";

constexpr std::string_view kAclStageIngress = "ingress";
constexpr std::string_view kAclStageIngressMacsec = "ingress-macsec";
constexpr std::string_view kAclStageEgressMacsec = "egress-macsec";
constexpr std::string_view kAclStageIngressPostLookup = "ingress-post-lookup";

const std::unordered_map<std::string_view, cfg::AclStage> kAclStageByName = {
    {kAclStageIngress, cfg::AclStage::INGRESS},
    {kAclStageIngressMacsec, cfg::AclStage::INGRESS_MACSEC},
    {kAclStageEgressMacsec, cfg::AclStage::EGRESS_MACSEC},
    {kAclStageIngressPostLookup, cfg::AclStage::INGRESS_POST_LOOKUP},
};

cfg::AclStage parseAclStage(const std::string& s) {
  // Try named string first (case-insensitive via tolower).
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  auto it = kAclStageByName.find(lower);
  if (it != kAclStageByName.end()) {
    return it->second;
  }
  // Numeric fallback.
  try {
    int n = folly::to<int>(s);
    switch (n) {
      case 0:
        return cfg::AclStage::INGRESS;
      case 1:
        return cfg::AclStage::INGRESS_MACSEC;
      case 2:
        return cfg::AclStage::EGRESS_MACSEC;
      case 3:
        return cfg::AclStage::INGRESS_POST_LOOKUP;
      default:
        break;
    }
  } catch (const folly::ConversionError&) {
  }
  throw std::invalid_argument(
      fmt::format(
          "Invalid stage '{}'. Valid values: ingress, ingress-macsec, "
          "egress-macsec, ingress-post-lookup (or numeric 0-3)",
          s));
}
} // namespace

AclTableGroupConfigArgs::AclTableGroupConfigArgs(std::vector<std::string> v) {
  if (v.size() != 3) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <group-name> stage <stage-value>, got {} argument(s)",
            v.size()));
  }
  if (v[1] != kAclTableGroupAttrStage) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown attribute '{}' for acl table-group. Valid attrs: stage",
            v[1]));
  }
  groupName_ = v[0];
  stage_ = parseAclStage(v[2]);
  data_ = std::move(v);
}

CmdConfigAclTableGroupTraits::RetType CmdConfigAclTableGroup::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  auto& group =
      acl_utils::findAclTableGroupOrThrow(swConfig, args.getGroupName());
  group.stage() = args.getStage();

  // ACL table group changes are applied at runtime via
  // processAclTableGroupDelta (SaiAclTableGroupManager); no change-prohibited
  // guard exists in SaiSwitch.
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  return fmt::format(
      "Set acl table-group '{}' stage to {}",
      args.getGroupName(),
      apache::thrift::util::enumNameSafe(args.getStage()));
}

void CmdConfigAclTableGroup::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigAclTableGroup, CmdConfigAclTableGroupTraits>::run();

} // namespace facebook::fboss
