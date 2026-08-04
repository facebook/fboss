/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/CmdConfigProtocolBgpPolicyAsPathList.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <cstdint>
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

// CLI keyword selecting the nested entry, and the attribute names, exactly as
// documented. Kept here so the valid-attribute sets and the handler tables
// stay in sync.
constexpr std::string_view kEntryKeyword = "entry";
constexpr std::string_view kDescription = "description";
constexpr std::string_view kAsnRegexp = "asn-regexp";
constexpr std::string_view kMatchLogic = "match-logic";

// match-logic values (routing_policy.MatchValueLogicOperator names).
constexpr std::string_view kMatchLogicEqual = "EQUAL";
constexpr std::string_view kMatchLogicNotEqual = "NOT_EQUAL";

using AsPathList = bgp::bgp_policy::AsPathList;
using AsPathListEntry = bgp::bgp_policy::AsPathListEntry;
using MatchValueLogicOperator = bgp::routing_policy::MatchValueLogicOperator;
using bgpcli::AttrHandler;
using bgpcli::enumAttr;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::Result;
using bgpcli::Tokens;

// ---- list-level attribute handlers ----------------------------------------
const std::map<std::string, AttrHandler<AsPathList>, std::less<>>&
listAttrHandlers() {
  static const std::map<std::string, AttrHandler<AsPathList>, std::less<>>
      kHandlers = {
          {std::string(kDescription),
           joinedStringAttr<AsPathList>(
               kDescription,
               [](AsPathList& l, const std::string& v) {
                 l.description() = v;
               })},
      };
  return kHandlers;
}

// ---- entry-level attribute handlers ----------------------------------------
const std::map<std::string, AttrHandler<AsPathListEntry>, std::less<>>&
entryAttrHandlers() {
  static const std::map<std::string, AttrHandler<AsPathListEntry>, std::less<>>
      kHandlers = {
          {std::string(kDescription),
           joinedStringAttr<AsPathListEntry>(
               kDescription,
               [](AsPathListEntry& e, const std::string& v) {
                 e.description() = v;
               })},
          // The AS path is modelled as an AsPathType union; the CLI sets the
          // inline `as_path` (AsPath) arm and its regexp, matching the thrift
          // path .as_path.as_path.asn_regexp. The pattern may contain spaces
          // (AS-path regexes separate ASNs with spaces, e.g. `^65000 65001$`),
          // so the tokens are re-joined rather than required to be single.
          {std::string(kAsnRegexp),
           joinedStringAttr<AsPathListEntry>(
               kAsnRegexp,
               [](AsPathListEntry& e, const std::string& v) {
                 e.as_path()->set_as_path().asn_regexp() = v;
               })},
          {std::string(kMatchLogic),
           enumAttr<AsPathListEntry, MatchValueLogicOperator>(
               kMatchLogic,
               fmt::format("{}|{}", kMatchLogicEqual, kMatchLogicNotEqual),
               [](const std::string& s)
                   -> std::optional<MatchValueLogicOperator> {
                 if (s == kMatchLogicEqual) {
                   return MatchValueLogicOperator::EQUAL;
                 }
                 if (s == kMatchLogicNotEqual) {
                   return MatchValueLogicOperator::NOT_EQUAL;
                 }
                 return std::nullopt;
               },
               [](AsPathListEntry& e, MatchValueLogicOperator op) {
                 e.match_logic_type() = op;
               })},
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

// Find the as-path-list keyed by name, creating it if absent. Setting an
// attribute on a not-yet-created list implicitly creates it, so command
// ordering stays forgiving; a bare `as-path-list <name>` creates one
// explicitly. AsPathList's only key field is `name`.
AsPathList& findOrCreateList(
    bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  auto& lists = *cfg.policies().ensure().aspath_lists();
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

// Find the entry keyed by sequence_number within a list, creating it if
// absent. sequence_number is the entry's identity.
AsPathListEntry& findOrCreateEntry(AsPathList& list, int64_t seqNum) {
  auto& entries = *list.as_path_list();
  for (auto& entry : entries) {
    if (entry.sequence_number().has_value() &&
        *entry.sequence_number() == seqNum) {
      return entry;
    }
  }
  entries.emplace_back();
  auto& entry = entries.back();
  entry.sequence_number() = seqNum;
  return entry;
}

bool asPathListExists(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  if (!cfg.policies().has_value()) {
    return false;
  }
  for (const auto& list : *cfg.policies()->aspath_lists()) {
    if (*list.name() == name) {
      return true;
    }
  }
  return false;
}

bool asPathListEntryExists(const AsPathList& list, int64_t seqNum) {
  for (const auto& entry : *list.as_path_list()) {
    if (entry.sequence_number().has_value() &&
        *entry.sequence_number() == seqNum) {
      return true;
    }
  }
  return false;
}

} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
// Throwing std::invalid_argument is how the framework surfaces arg parse
// errors (same mechanism as BgpPeerGroupConfig).
BgpAsPathListConfig::BgpAsPathListConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: as-path-list <name> is required, optionally followed by "
        "`entry <seq-num>` and an <attribute> <value>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument("Error: as-path-list name must not be empty");
  }
  listName_ = v[0];

  if (v.size() == 1) {
    return; // bare `as-path-list <name>`: create the list
  }

  // `entry <seq-num>` selects the nested entry; anything else is a list-level
  // attribute.
  size_t attrStart = 1;
  if (v[1] == kEntryKeyword) {
    if (v.size() < 3) {
      throw std::invalid_argument("Error: `entry` requires a <seq-num>");
    }
    auto seq = bgpcli::parseNonNegInt64(v[2]);
    if (!seq) {
      throw std::invalid_argument(
          fmt::format(
              "Error: entry <seq-num> must be a non-negative integer, got '{}'",
              v[2]));
    }
    hasEntry_ = true;
    seqNum_ = *seq;
    if (v.size() == 3) {
      return; // bare `as-path-list <name> entry <seq-num>`: create the entry
    }
    attrStart = 3;
  }

  attr_ = v[attrStart];
  values_.assign(v.begin() + attrStart + 1, v.end());

  const bool known = hasEntry_
      ? entryAttrHandlers().find(attr_) != entryAttrHandlers().end()
      : listAttrHandlers().find(attr_) != listAttrHandlers().end();
  if (!known) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unknown as-path-list {} attribute '{}'. Valid "
            "attributes: {}",
            hasEntry_ ? "entry" : "list",
            attr_,
            hasEntry_ ? validAttrList(entryAttrHandlers())
                      : validAttrList(listAttrHandlers())));
  }
}

CmdConfigProtocolBgpPolicyAsPathListTraits::RetType
CmdConfigProtocolBgpPolicyAsPathList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool listCreated = !asPathListExists(cfg, args.listName());
  auto& list = findOrCreateList(cfg, args.listName());

  Result result;
  if (!args.hasEntry()) {
    result = args.attr().empty()
        ? ok(listCreated
                 ? fmt::format(
                       "Successfully created BGP as-path-list {}",
                       args.listName())
                 : fmt::format(
                       "BGP as-path-list {} already exists", args.listName()))
        // The attribute is guaranteed valid: BgpAsPathListConfig's constructor
        // rejects an unknown attribute before we get here.
        : listAttrHandlers().find(args.attr())->second(list, args.values());
  } else {
    const bool entryCreated = !asPathListEntryExists(list, args.seqNum());
    auto& entry = findOrCreateEntry(list, args.seqNum());
    result = args.attr().empty()
        ? ok(entryCreated
                 ? fmt::format(
                       "Successfully created BGP as-path-list {} entry {}",
                       args.listName(),
                       args.seqNum())
                 : fmt::format(
                       "BGP as-path-list {} entry {} already exists",
                       args.listName(),
                       args.seqNum()))
        : entryAttrHandlers().find(args.attr())->second(entry, args.values());
    // A rejected value must not leave a phantom entry behind. If the list was
    // freshly created too, the list-level rollback below removes it (and the
    // entry with it), so only roll back the entry when the list pre-existed.
    if (!result.ok && entryCreated && !listCreated) {
      list.as_path_list()->pop_back();
    }
  }

  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += args.hasEntry()
          ? fmt::format(
                " for as-path-list {} entry {}", args.listName(), args.seqNum())
          : fmt::format(" for as-path-list {}", args.listName());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else if (listCreated) {
    // Drop the phantom list (and any entry created under it this invocation)
    // so a rejected value is not visible to later lookups in the same process.
    cfg.policies()->aspath_lists()->pop_back();
  }
  return result.message;
}

void CmdConfigProtocolBgpPolicyAsPathList::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigProtocolBgpPolicyAsPathList,
    CmdConfigProtocolBgpPolicyAsPathListTraits>::run();

} // namespace facebook::fboss
