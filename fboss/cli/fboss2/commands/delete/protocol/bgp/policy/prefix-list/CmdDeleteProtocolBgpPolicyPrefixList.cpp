/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/prefix-list/CmdDeleteProtocolBgpPolicyPrefixList.h"

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
// CLI keyword selecting the nested entry, matching the config command's
// grammar.
constexpr std::string_view kObjectName = "prefix-list";
constexpr std::string_view kEntryKeyword = "entry";
} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
BgpPrefixListRef::BgpPrefixListRef(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  // Pre-empt the generic member-selector message for a missing seq-num: the
  // shared parser would call the member a <name>, but here it is a number.
  if (v.size() >= 2 && v[1] == kEntryKeyword &&
      (v.size() < 3 || v[2].empty())) {
    throw std::invalid_argument("Error: `entry` requires a <seq-num>");
  }
  auto selector = bgpcli::parseListMemberSelector(
      v,
      kObjectName,
      kEntryKeyword,
      "Error: delete protocol bgp policy prefix-list requires <name>, "
      "optionally followed by `entry <seq-num>`");
  // Unlike the config grammar, nothing may follow the parsed prefix: there
  // are no attributes to delete through this command.
  if (selector.restStart < v.size()) {
    throw std::invalid_argument(
        selector.memberName
            ? fmt::format(
                  "Error: unexpected token '{}' after entry <seq-num>",
                  v[selector.restStart])
            : fmt::format(
                  "Error: unexpected token '{}'. Usage: delete protocol bgp "
                  "policy prefix-list <name> [entry <seq-num>]",
                  v[selector.restStart]));
  }
  listName_ = std::move(selector.listName);
  if (selector.memberName) {
    auto seq = bgpcli::parseNonNegInt32(*selector.memberName);
    if (!seq) {
      throw std::invalid_argument(
          fmt::format(
              "Error: entry <seq-num> must be a non-negative integer, got '{}'",
              *selector.memberName));
    }
    hasEntry_ = true;
    seqNum_ = *seq;
  }
}

CmdDeleteProtocolBgpPolicyPrefixListTraits::RetType
CmdDeleteProtocolBgpPolicyPrefixList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  if (!cfg.policies().has_value()) {
    // Nothing is persisted for an unknown list, so a typo'd delete can't stage
    // an unrelated session change.
    return fmt::format("Error: BGP prefix-list {} not found", args.listName());
  }
  auto& lists = *cfg.policies()->prefix_lists();
  auto it = std::find_if(lists.begin(), lists.end(), [&](const auto& list) {
    return *list.name() == args.listName();
  });
  if (it == lists.end()) {
    return fmt::format("Error: BGP prefix-list {} not found", args.listName());
  }
  if (args.hasEntry()) {
    // Delete a single entry; the list itself stays.
    auto& entries = *it->prefixes();
    auto entryIt =
        std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
          return entry.seq_num().has_value() &&
              *entry.seq_num() == args.seqNum();
        });
    if (entryIt == entries.end()) {
      return fmt::format(
          "Error: BGP prefix-list {} entry {} not found",
          args.listName(),
          args.seqNum());
    }
    entries.erase(entryIt);
    session.saveBgpConfig();
    return fmt::format(
        "Successfully deleted BGP prefix-list {} entry {}\n"
        "Config saved to: {}",
        args.listName(),
        args.seqNum(),
        session.getBgpSessionConfigPath());
  }
  lists.erase(it);
  session.saveBgpConfig();
  return fmt::format(
      "Successfully deleted BGP prefix-list {}\nConfig saved to: {}",
      args.listName(),
      session.getBgpSessionConfigPath());
}

void CmdDeleteProtocolBgpPolicyPrefixList::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdDeleteProtocolBgpPolicyPrefixList,
    CmdDeleteProtocolBgpPolicyPrefixListTraits>::run();

} // namespace facebook::fboss
