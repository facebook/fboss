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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliValueParsers.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {

// CLI keyword selecting the nested community member, and the attribute names,
// exactly as documented. Kept here so the valid-attribute sets and the handler
// tables stay in sync.
constexpr std::string_view kObjectName = "community-list";
constexpr std::string_view kCommunityKeyword = "community";
constexpr std::string_view kBooleanOperator = "boolean-operator";
constexpr std::string_view kDescription = "description";
constexpr std::string_view kExactMatch = "exact-match";
constexpr std::string_view kType = "type";
constexpr std::string_view kValue = "value";

// boolean-operator values (routing_policy.BooleanOperator names).
constexpr std::string_view kBooleanOperatorAnd = "AND";
constexpr std::string_view kBooleanOperatorOr = "OR";
constexpr std::string_view kBooleanOperatorNot = "NOT";

// type values (bgp_policy.CommunityType names).
constexpr std::string_view kCommunityTypeNormal = "NORMAL";
constexpr std::string_view kCommunityTypeExtended = "EXTENDED";
constexpr std::string_view kCommunityTypeLarge = "LARGE";

using BooleanOperator = bgp::routing_policy::BooleanOperator;
using Community = bgp::bgp_policy::Community;
using CommunityList = bgp::bgp_policy::CommunityList;
using CommunityType = bgp::bgp_policy::CommunityType;
using bgpcli::AttrHandler;
using bgpcli::boolAttr;
using bgpcli::enumAttr;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::Result;
using bgpcli::stringAttr;
using bgpcli::Tokens;

// ---- list-level attribute handlers ----------------------------------------
const std::map<std::string, AttrHandler<CommunityList>, std::less<>>&
listAttrHandlers() {
  static const std::map<std::string, AttrHandler<CommunityList>, std::less<>>
      kHandlers = {
          {std::string(kBooleanOperator),
           enumAttr<CommunityList, BooleanOperator>(
               kBooleanOperator,
               fmt::format(
                   "{}|{}|{}",
                   kBooleanOperatorAnd,
                   kBooleanOperatorOr,
                   kBooleanOperatorNot),
               [](const std::string& s) -> std::optional<BooleanOperator> {
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
               },
               [](CommunityList& l, BooleanOperator op) {
                 l.boolean_operator() = op;
               })},
          {std::string(kDescription),
           joinedStringAttr<CommunityList>(
               kDescription,
               [](CommunityList& l, const std::string& v) {
                 l.description() = v;
               })},
          {std::string(kExactMatch),
           boolAttr<CommunityList>(
               kExactMatch,
               [](CommunityList& l, bool v) { l.exact_match() = v; })},
      };
  return kHandlers;
}

// ---- community (member)-level attribute handlers ---------------------------
const std::map<std::string, AttrHandler<Community>, std::less<>>&
communityAttrHandlers() {
  static const std::map<std::string, AttrHandler<Community>, std::less<>>
      kHandlers = {
          {std::string(kDescription),
           joinedStringAttr<Community>(
               kDescription,
               [](Community& c, const std::string& v) {
                 c.description() = v;
               })},
          {std::string(kType),
           enumAttr<Community, CommunityType>(
               kType,
               fmt::format(
                   "{}|{}|{}",
                   kCommunityTypeNormal,
                   kCommunityTypeExtended,
                   kCommunityTypeLarge),
               [](const std::string& s) -> std::optional<CommunityType> {
                 if (s == kCommunityTypeNormal) {
                   return CommunityType::NORMAL;
                 }
                 if (s == kCommunityTypeExtended) {
                   return CommunityType::EXTENDED;
                 }
                 if (s == kCommunityTypeLarge) {
                   return CommunityType::LARGE;
                 }
                 return std::nullopt;
               },
               [](Community& c, CommunityType t) { c.type() = t; })},
          {std::string(kValue),
           stringAttr<Community>(
               kValue,
               "string",
               [](Community& c, const std::string& v) { c.value() = v; })},
      };
  return kHandlers;
}

template <typename Handlers>
std::string validAttrList(const Handlers& handlers) {
  std::string out;
  for (const auto& [name, _] : handlers) {
    if (!out.empty()) {
      out += ", ";
    }
    out += name;
  }
  return out;
}

// Find the community-list keyed by name, creating it if absent. Setting an
// attribute on a not-yet-created list implicitly creates it, so command
// ordering stays forgiving; a bare `community-list <name>` creates one
// explicitly. CommunityList's only key field is `name`.
CommunityList& findOrCreateList(
    bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  auto& lists = *cfg.policies().ensure().community_lists();
  for (auto& list : lists) {
    if (*list.name() == name) {
      return list;
    }
  }
  lists.emplace_back();
  auto& list = lists.back();
  list.name() = name;
  return list;
}

// Find the inline Community member keyed by name within a list, creating it
// if absent. The CLI always defines members inline (the CommunityRefType
// union's `community` arm); the member's name is its identity.
Community& findOrCreateMember(CommunityList& list, const std::string& name) {
  auto& members = list.members().ensure();
  for (auto& member : members) {
    if (member.community_ref().has_value() &&
        *member.community_ref()->name() == name) {
      return *member.community_ref();
    }
  }
  members.emplace_back();
  auto& community = members.back().set_community();
  community.name() = name;
  return community;
}

bool communityListExists(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  if (!cfg.policies().has_value()) {
    return false;
  }
  for (const auto& list : *cfg.policies()->community_lists()) {
    if (*list.name() == name) {
      return true;
    }
  }
  return false;
}

bool communityMemberExists(const CommunityList& list, const std::string& name) {
  if (!list.members().has_value()) {
    return false;
  }
  for (const auto& member : *list.members()) {
    if (member.community_ref().has_value() &&
        *member.community_ref()->name() == name) {
      return true;
    }
  }
  return false;
}

} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
// Throwing std::invalid_argument is how the framework surfaces arg parse
// errors (same mechanism as BgpAsPathListConfig).
BgpCommunityListConfig::BgpCommunityListConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  // `community <name>` selects the nested member; anything else after the
  // list name is a list-level attribute.
  auto selector = bgpcli::parseListMemberSelector(
      v,
      kObjectName,
      kCommunityKeyword,
      "Error: community-list <name> is required, optionally followed by "
      "`community <name>` and an <attribute> <value>");
  listName_ = std::move(selector.listName);
  if (selector.memberName) {
    hasCommunity_ = true;
    communityName_ = std::move(*selector.memberName);
  }
  if (selector.restStart == v.size()) {
    return; // bare `community-list <name> [community <name>]`: create it
  }

  attr_ = v[selector.restStart];
  values_.assign(v.begin() + selector.restStart + 1, v.end());

  const bool known = hasCommunity_
      ? communityAttrHandlers().find(attr_) != communityAttrHandlers().end()
      : listAttrHandlers().find(attr_) != listAttrHandlers().end();
  if (!known) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unknown community-list {} attribute '{}'. Valid "
            "attributes: {}",
            hasCommunity_ ? "community" : "list",
            attr_,
            hasCommunity_ ? validAttrList(communityAttrHandlers())
                          : validAttrList(listAttrHandlers())));
  }
}

CmdConfigProtocolBgpPolicyCommunityListTraits::RetType
CmdConfigProtocolBgpPolicyCommunityList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool listCreated = !communityListExists(cfg, args.listName());
  auto& list = findOrCreateList(cfg, args.listName());

  Result result;
  if (!args.hasCommunity()) {
    result = args.attr().empty()
        ? ok(listCreated
                 ? fmt::format(
                       "Successfully created BGP community-list {}",
                       args.listName())
                 : fmt::format(
                       "BGP community-list {} already exists", args.listName()))
        // The attribute is guaranteed valid: BgpCommunityListConfig's
        // constructor rejects an unknown attribute before we get here.
        : listAttrHandlers().find(args.attr())->second(list, args.values());
  } else {
    const bool hadMembers = list.members().has_value();
    const bool memberCreated =
        !communityMemberExists(list, args.communityName());
    auto& community = findOrCreateMember(list, args.communityName());
    result = args.attr().empty()
        ? ok(memberCreated
                 ? fmt::format(
                       "Successfully created BGP community-list {} "
                       "community {}",
                       args.listName(),
                       args.communityName())
                 : fmt::format(
                       "BGP community-list {} community {} already exists",
                       args.listName(),
                       args.communityName()))
        : communityAttrHandlers()
              .find(args.attr())
              ->second(community, args.values());
    // A rejected value must not leave a phantom member behind. If the list
    // was freshly created too, the list-level rollback below removes it (and
    // the member with it), so only roll back the member when the list
    // pre-existed — restoring an unset members field if this invocation
    // ensured it.
    if (!result.ok && memberCreated && !listCreated) {
      if (hadMembers) {
        list.members()->pop_back();
      } else {
        list.members().reset();
      }
    }
  }

  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += args.hasCommunity()
          ? fmt::format(
                " for community-list {} community {}",
                args.listName(),
                args.communityName())
          : fmt::format(" for community-list {}", args.listName());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else if (listCreated) {
    // Drop the phantom list (and any member created under it this invocation)
    // so a rejected value is not visible to later lookups in the same process.
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
