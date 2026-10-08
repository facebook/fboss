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
#include <folly/String.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

#ifndef IS_OSS
#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#else
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#endif

namespace facebook::fboss {

namespace {
// Every routing-policy term whose PREFIX_LIST match names `listName` in
// prefix_filters.prefix_list_names (the field bgpd resolves at config load),
// labelled the way the CLI addresses the term.
std::vector<std::string> findTermsReferencingPrefixList(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& listName) {
  std::vector<std::string> referrers;
  if (!cfg.policies().has_value()) {
    return referrers;
  }
  for (const auto& policy : *cfg.policies()->bgp_policy_statements()) {
    for (const auto& term : *policy.policy_entries()) {
      if (!term.policy_match_entries().has_value()) {
        continue;
      }
      for (const auto& match : *term.policy_match_entries()->match_entries()) {
        if (!match.prefix_filters().has_value()) {
          continue;
        }
        const auto& names = *match.prefix_filters()->prefix_list_names();
        if (std::find(names.begin(), names.end(), listName) == names.end()) {
          continue;
        }
        referrers.push_back(
            term.sequence_number().has_value()
                ? fmt::format(
                      "policy {} term {}",
                      *policy.name(),
                      *term.sequence_number())
                : fmt::format(
                      "policy {} term {}", *policy.name(), *term.name()));
      }
    }
  }
  return referrers;
}
} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
BgpPrefixListRef::BgpPrefixListRef(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: delete protocol bgp policy prefix-list requires <name>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument("Error: prefix-list name must not be empty");
  }
  // Nothing may follow the name: there are no attributes to delete through
  // this command, and entries are deleted through the entry subcommand.
  if (v.size() > 1) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unexpected token '{}'. Usage: delete protocol bgp policy "
            "prefix-list <name>",
            v[1]));
  }
  listName_ = v[0];
}

CmdDeleteProtocolBgpPolicyPrefixListTraits::RetType
CmdDeleteProtocolBgpPolicyPrefixList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  // Delete mirrors add: an absent target is already the requested end state,
  // so this is a success with a warning. Nothing is saved, so a typo'd delete
  // can't stage an unrelated session change.
  auto absent = fmt::format(
      "Warning: BGP prefix-list {} does not exist; nothing to delete",
      args.listName());
  if (!cfg.policies().has_value()) {
    return absent;
  }
  auto& lists = *cfg.policies()->prefix_lists();
  auto it = std::find_if(lists.begin(), lists.end(), [&](const auto& list) {
    return *list.name() == args.listName();
  });
  if (it == lists.end()) {
    return absent;
  }
  // A term's prefix_list_names resolves against this list by name at daemon
  // load; erasing the list while a term still names it would commit a
  // dangling reference and fail the load. Refuse and name the terms instead.
  auto referrers = findTermsReferencingPrefixList(cfg, args.listName());
  if (!referrers.empty()) {
    return fmt::format(
        "Error: BGP prefix-list {} is still referenced by {}; remove those "
        "matches first",
        args.listName(),
        folly::join(", ", referrers));
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
