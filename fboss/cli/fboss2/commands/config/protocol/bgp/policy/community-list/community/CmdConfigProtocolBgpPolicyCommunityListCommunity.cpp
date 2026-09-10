/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/community/CmdConfigProtocolBgpPolicyCommunityListCommunity.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliAttrHandlers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/BgpCommunityListCliUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {

// The attribute names, exactly as documented. Kept here so the
// valid-attribute set and the handler table stay in sync.
constexpr std::string_view kDescription = "description";
constexpr std::string_view kType = "type";
constexpr std::string_view kValue = "value";

// type values (bgp_policy.CommunityType names).
constexpr std::string_view kCommunityTypeNormal = "NORMAL";
constexpr std::string_view kCommunityTypeExtended = "EXTENDED";
constexpr std::string_view kCommunityTypeLarge = "LARGE";

using Community = bgp::bgp_policy::Community;
using CommunityType = bgp::bgp_policy::CommunityType;
using bgpcli::AttrHandler;
using bgpcli::enumAttr;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::Result;
using bgpcli::stringAttr;

// ---- value lookups ----------------------------------------------------------

std::optional<CommunityType> lookupCommunityType(const std::string& s) {
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
}

// ---- community-level setters ------------------------------------------------
// Each writes one already-parsed, already-validated value. Parsing and message
// text belong to the shared factories in BgpCliAttrHandlers.h.

void setDescription(Community& community, const std::string& description) {
  community.description() = description;
}

void setType(Community& community, CommunityType type) {
  community.type() = type;
}

void setValue(Community& community, const std::string& value) {
  community.value() = value;
}

// ---- community-level attribute registry -------------------------------------
// One line per documented attribute: its dispatch key, its value shape, and
// the setter that stores it.
const std::map<std::string, AttrHandler<Community>, std::less<>>&
communityAttrHandlers() {
  static const std::string kCommunityTypeValues = fmt::format(
      "{}|{}|{}",
      kCommunityTypeNormal,
      kCommunityTypeExtended,
      kCommunityTypeLarge);
  static const std::map<std::string, AttrHandler<Community>, std::less<>>
      kHandlers = {
          {std::string(kDescription),
           joinedStringAttr<Community>(kDescription, setDescription)},
          {std::string(kType),
           enumAttr<Community, CommunityType>(
               kType, kCommunityTypeValues, lookupCommunityType, setType)},
          {std::string(kValue),
           stringAttr<Community>(kValue, "string", setValue)},
      };
  return kHandlers;
}

std::string validAttrList() {
  std::string out;
  for (const auto& [name, _] : communityAttrHandlers()) {
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
// errors (same mechanism as BgpCommunityListConfig).
BgpCommunityListCommunityConfig::BgpCommunityListCommunityConfig(
    std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty() || v[0].empty()) {
    throw std::invalid_argument(
        "Error: community <name> is required, optionally followed by an "
        "<attribute> <value>");
  }
  communityName_ = v[0];
  if (v.size() == 1) {
    return; // bare `community <name>`: create it
  }

  attr_ = v[1];
  values_.assign(v.begin() + 2, v.end());

  if (communityAttrHandlers().find(attr_) == communityAttrHandlers().end()) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unknown community-list community attribute '{}'. Valid "
            "attributes: {}",
            attr_,
            validAttrList()));
  }
}

CmdConfigProtocolBgpPolicyCommunityListCommunityTraits::RetType
CmdConfigProtocolBgpPolicyCommunityListCommunity::queryClient(
    const HostInfo& /* hostInfo */,
    const BgpCommunityListConfig& listArgs,
    const ObjectArgType& args) {
  // The parent parse accepts `community-list <name> <attr> <value> ...
  // community ...`, but only the leaf (this command) runs — silently dropping
  // the list-level attribute would look like it was staged. Reject the mix.
  if (!listArgs.attr().empty()) {
    return fmt::format(
        "Error: configure community-list attributes and community in separate "
        "commands (got community-list attribute '{}' alongside community {})",
        listArgs.attr(),
        args.communityName());
  }

  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool listCreated =
      !bgpcli::communityListExists(cfg, listArgs.listName());
  auto& list = bgpcli::findOrCreateCommunityList(cfg, listArgs.listName());
  const bool hadMembers = list.members().has_value();
  const bool memberCreated =
      !bgpcli::communityMemberExists(list, args.communityName());
  auto& community =
      bgpcli::findOrCreateCommunityMember(list, args.communityName());

  Result result = args.attr().empty()
      ? ok(memberCreated
               ? fmt::format(
                     "Successfully created BGP community-list {} community {}",
                     listArgs.listName(),
                     args.communityName())
               : fmt::format(
                     "BGP community-list {} community {} already exists",
                     listArgs.listName(),
                     args.communityName()))
      // The attribute is guaranteed valid: BgpCommunityListCommunityConfig's
      // constructor rejects an unknown attribute before we get here.
      : communityAttrHandlers()
            .find(args.attr())
            ->second(community, args.values());

  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += fmt::format(
          " for community-list {} community {}",
          listArgs.listName(),
          args.communityName());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else {
    // A rejected value must not leave a phantom member (or a phantom list
    // implicitly created for it) visible to later lookups in this process —
    // restoring an unset members field if this invocation ensured it.
    if (memberCreated) {
      if (hadMembers) {
        list.members()->pop_back();
      } else {
        list.members().reset();
      }
    }
    if (listCreated) {
      cfg.policies()->community_lists()->pop_back();
    }
  }
  return result.message;
}

void CmdConfigProtocolBgpPolicyCommunityListCommunity::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigProtocolBgpPolicyCommunityListCommunity,
    CmdConfigProtocolBgpPolicyCommunityListCommunityTraits>::run();

} // namespace facebook::fboss
