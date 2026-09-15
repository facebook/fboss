/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/community-list/CmdDeleteProtocolBgpPolicyCommunityList.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <algorithm>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliValueParsers.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {
// CLI keyword selecting the nested community member, matching the config
// command's grammar.
constexpr std::string_view kObjectName = "community-list";
constexpr std::string_view kCommunityKeyword = "community";
} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
BgpCommunityListRef::BgpCommunityListRef(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  auto selector = bgpcli::parseListMemberSelector(
      v,
      kObjectName,
      kCommunityKeyword,
      "Error: delete protocol bgp policy community-list requires <name>, "
      "optionally followed by `community <name>`");
  // Unlike the config grammar, nothing may follow the parsed prefix: there
  // are no attributes to delete through this command.
  if (selector.restStart < v.size()) {
    throw std::invalid_argument(
        selector.memberName
            ? fmt::format(
                  "Error: unexpected token '{}' after community <name>",
                  v[selector.restStart])
            : fmt::format(
                  "Error: unexpected token '{}'. Usage: delete protocol bgp "
                  "policy community-list <name> [community <name>]",
                  v[selector.restStart]));
  }
  listName_ = std::move(selector.listName);
  if (selector.memberName) {
    hasCommunity_ = true;
    communityName_ = std::move(*selector.memberName);
  }
}

CmdDeleteProtocolBgpPolicyCommunityListTraits::RetType
CmdDeleteProtocolBgpPolicyCommunityList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  if (!cfg.policies().has_value()) {
    // Nothing is persisted for an unknown list, so a typo'd delete can't stage
    // an unrelated session change.
    return fmt::format(
        "Error: BGP community-list {} not found", args.listName());
  }
  auto& lists = *cfg.policies()->community_lists();
  auto it = std::find_if(lists.begin(), lists.end(), [&](const auto& list) {
    return *list.name() == args.listName();
  });
  if (it == lists.end()) {
    return fmt::format(
        "Error: BGP community-list {} not found", args.listName());
  }
  if (args.hasCommunity()) {
    // Delete a single inline member; the list itself stays.
    auto& list = *it;
    if (list.members().has_value()) {
      auto& members = *list.members();
      auto memberIt =
          std::find_if(members.begin(), members.end(), [&](const auto& member) {
            return member.community_ref().has_value() &&
                *member.community_ref()->name() == args.communityName();
          });
      if (memberIt != members.end()) {
        members.erase(memberIt);
        if (members.empty()) {
          // Leave the list looking like one that never had members, so a
          // later `community <name>` add starts from the same shape.
          list.members().reset();
        }
        session.saveBgpConfig();
        return fmt::format(
            "Successfully deleted BGP community-list {} community {}\n"
            "Config saved to: {}",
            args.listName(),
            args.communityName(),
            session.getBgpSessionConfigPath());
      }
    }
    return fmt::format(
        "Error: BGP community-list {} community {} not found",
        args.listName(),
        args.communityName());
  }
  lists.erase(it);
  session.saveBgpConfig();
  return fmt::format(
      "Successfully deleted BGP community-list {}\nConfig saved to: {}",
      args.listName(),
      session.getBgpSessionConfigPath());
}

void CmdDeleteProtocolBgpPolicyCommunityList::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdDeleteProtocolBgpPolicyCommunityList,
    CmdDeleteProtocolBgpPolicyCommunityListTraits>::run();

} // namespace facebook::fboss
