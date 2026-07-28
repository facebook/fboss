/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/CmdConfigProtocolBgpPolicyCommunityList.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliAttrHandlers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/BgpCommunityListCliUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {

// The attribute names, exactly as documented. Kept here so the
// valid-attribute set and the handler table stay in sync. The list's inline
// Community members are their own subcommand, not attributes.
constexpr std::string_view kBooleanOperator = "boolean-operator";
constexpr std::string_view kDescription = "description";
constexpr std::string_view kExactMatch = "exact-match";

// boolean-operator values (routing_policy.BooleanOperator names).
constexpr std::string_view kBooleanOperatorAnd = "AND";
constexpr std::string_view kBooleanOperatorOr = "OR";
constexpr std::string_view kBooleanOperatorNot = "NOT";

using BooleanOperator = bgp::routing_policy::BooleanOperator;
using CommunityList = bgp::bgp_policy::CommunityList;
using bgpcli::AttrHandler;
using bgpcli::boolAttr;
using bgpcli::enumAttr;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::Result;

// ---- value lookups ----------------------------------------------------------

std::optional<BooleanOperator> lookupBooleanOperator(const std::string& s) {
  if (s == kBooleanOperatorAnd) {
    return BooleanOperator::AND;
  }
  if (s == kBooleanOperatorOr) {
    return BooleanOperator::OR;
  }
  if (s == kBooleanOperatorNot) {
    return BooleanOperator::NOT;
  }
  return std::nullopt;
}

// ---- list-level setters -----------------------------------------------------
// Each writes one already-parsed, already-validated value. Parsing and message
// text belong to the shared factories in BgpCliAttrHandlers.h.

void setListBooleanOperator(CommunityList& list, BooleanOperator op) {
  list.boolean_operator() = op;
}

void setListDescription(CommunityList& list, const std::string& description) {
  list.description() = description;
}

void setListExactMatch(CommunityList& list, bool exactMatch) {
  list.exact_match() = exactMatch;
}

// ---- list-level attribute registry ------------------------------------------
// One line per documented attribute: its dispatch key, its value shape, and
// the setter that stores it.
const std::map<std::string, AttrHandler<CommunityList>, std::less<>>&
listAttrHandlers() {
  static const std::string kBooleanOperatorValues = fmt::format(
      "{}|{}|{}", kBooleanOperatorAnd, kBooleanOperatorOr, kBooleanOperatorNot);
  static const std::map<std::string, AttrHandler<CommunityList>, std::less<>>
      kHandlers = {
          {std::string(kBooleanOperator),
           enumAttr<CommunityList, BooleanOperator>(
               kBooleanOperator,
               kBooleanOperatorValues,
               lookupBooleanOperator,
               setListBooleanOperator)},
          {std::string(kDescription),
           joinedStringAttr<CommunityList>(kDescription, setListDescription)},
          {std::string(kExactMatch),
           boolAttr<CommunityList>(kExactMatch, setListExactMatch)},
      };
  return kHandlers;
}

std::string validAttrList() {
  std::string out;
  for (const auto& [name, _] : listAttrHandlers()) {
    if (!out.empty()) {
      out += ", ";
    }
    out += name;
  }
  return out;
}

} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
// Throwing std::invalid_argument is how the framework surfaces arg parse
// errors (same mechanism as BgpAsPathListConfig).
BgpCommunityListConfig::BgpCommunityListConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: community-list <name> is required, optionally followed by an "
        "<attribute> <value>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument("Error: community-list name must not be empty");
  }
  listName_ = v[0];

  if (v.size() == 1) {
    return; // bare `community-list <name>`: create the list
  }

  attr_ = v[1];
  values_.assign(v.begin() + 2, v.end());

  if (listAttrHandlers().find(attr_) == listAttrHandlers().end()) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unknown community-list attribute '{}'. Valid attributes: "
            "{}",
            attr_,
            validAttrList()));
  }
}

CmdConfigProtocolBgpPolicyCommunityListTraits::RetType
CmdConfigProtocolBgpPolicyCommunityList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool listCreated = !bgpcli::communityListExists(cfg, args.listName());
  auto& list = bgpcli::findOrCreateCommunityList(cfg, args.listName());

  Result result = args.attr().empty()
      ? ok(listCreated
               ? fmt::format(
                     "Successfully created BGP community-list {}",
                     args.listName())
               : fmt::format(
                     "BGP community-list {} already exists", args.listName()))
      // The attribute is guaranteed valid: BgpCommunityListConfig's
      // constructor rejects an unknown attribute before we get here.
      : listAttrHandlers().find(args.attr())->second(list, args.values());

  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += fmt::format(" for community-list {}", args.listName());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else if (listCreated) {
    // Drop the phantom list so a rejected value is not visible to later
    // lookups in the same process.
    cfg.policies()->community_lists()->pop_back();
  }
  return result.message;
}

void CmdConfigProtocolBgpPolicyCommunityList::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigProtocolBgpPolicyCommunityList,
    CmdConfigProtocolBgpPolicyCommunityListTraits>::run();

} // namespace facebook::fboss
