/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/entry/CmdConfigProtocolBgpPolicyAsPathListEntry.h"

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
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliAttrHandlers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliValueParsers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/BgpAsPathListCliUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {

// The attribute names, exactly as documented. Kept here so the
// valid-attribute set and the handler table stay in sync.
constexpr std::string_view kDescription = "description";
constexpr std::string_view kAsnRegexp = "asn-regexp";
constexpr std::string_view kMatchLogic = "match-logic";

// match-logic values (routing_policy.MatchValueLogicOperator names).
constexpr std::string_view kMatchLogicEqual = "EQUAL";
constexpr std::string_view kMatchLogicNotEqual = "NOT_EQUAL";

using AsPathListEntry = bgp::bgp_policy::AsPathListEntry;
using MatchValueLogicOperator = bgp::routing_policy::MatchValueLogicOperator;
using bgpcli::AttrHandler;
using bgpcli::enumAttr;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::Result;

// ---- value lookups ----------------------------------------------------------

std::optional<MatchValueLogicOperator> lookupMatchLogic(const std::string& s) {
  if (s == kMatchLogicEqual) {
    return MatchValueLogicOperator::EQUAL;
  }
  if (s == kMatchLogicNotEqual) {
    return MatchValueLogicOperator::NOT_EQUAL;
  }
  return std::nullopt;
}

// ---- entry-level setters ----------------------------------------------------
// Each writes one already-parsed, already-validated value. Parsing and message
// text belong to the shared factories in BgpCliAttrHandlers.h.

void setDescription(AsPathListEntry& entry, const std::string& description) {
  entry.description() = description;
}

// The AS path is modelled as an AsPathType union; the CLI sets the inline
// `as_path` (AsPath) arm and its regexp, matching the thrift path
// .as_path.as_path.asn_regexp.
void setAsnRegexp(AsPathListEntry& entry, const std::string& regexp) {
  entry.as_path()->set_as_path().asn_regexp() = regexp;
}

void setMatchLogic(AsPathListEntry& entry, MatchValueLogicOperator op) {
  entry.match_logic_type() = op;
}

// ---- entry-level attribute registry -----------------------------------------
// One line per documented attribute: its dispatch key, its value shape, and
// the setter that stores it. asn-regexp is a joined string because an AS-path
// regex separates ASNs with spaces (e.g. `^65000 65001$`), so its tokens must
// be re-joined rather than required to be single.
const std::map<std::string, AttrHandler<AsPathListEntry>, std::less<>>&
entryAttrHandlers() {
  static const std::string kMatchLogicValues =
      fmt::format("{}|{}", kMatchLogicEqual, kMatchLogicNotEqual);
  static const std::map<std::string, AttrHandler<AsPathListEntry>, std::less<>>
      kHandlers = {
          {std::string(kAsnRegexp),
           joinedStringAttr<AsPathListEntry>(kAsnRegexp, setAsnRegexp)},
          {std::string(kDescription),
           joinedStringAttr<AsPathListEntry>(kDescription, setDescription)},
          {std::string(kMatchLogic),
           enumAttr<AsPathListEntry, MatchValueLogicOperator>(
               kMatchLogic,
               kMatchLogicValues,
               lookupMatchLogic,
               setMatchLogic)},
      };
  return kHandlers;
}

std::string validAttrList() {
  std::string out;
  for (const auto& [name, _] : entryAttrHandlers()) {
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
BgpAsPathListEntryConfig::BgpAsPathListEntryConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: entry <seq-num> is required, optionally followed by an "
        "<attribute> <value>");
  }
  auto seq = bgpcli::parseNonNegInt64(v[0]);
  if (!seq) {
    throw std::invalid_argument(
        fmt::format(
            "Error: entry <seq-num> must be a non-negative integer, got '{}'",
            v[0]));
  }
  seqNum_ = *seq;
  if (v.size() == 1) {
    return; // bare `entry <seq-num>`: create it
  }

  attr_ = v[1];
  values_.assign(v.begin() + 2, v.end());

  if (entryAttrHandlers().find(attr_) == entryAttrHandlers().end()) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unknown as-path-list entry attribute '{}'. Valid "
            "attributes: {}",
            attr_,
            validAttrList()));
  }
}

CmdConfigProtocolBgpPolicyAsPathListEntryTraits::RetType
CmdConfigProtocolBgpPolicyAsPathListEntry::queryClient(
    const HostInfo& /* hostInfo */,
    const BgpAsPathListConfig& listArgs,
    const ObjectArgType& args) {
  // The parent parse accepts `as-path-list <name> <attr> <value> ... entry
  // ...`, but only the leaf (this command) runs — silently dropping the
  // list-level attribute would look like it was staged. Reject the mix.
  if (!listArgs.attr().empty()) {
    return fmt::format(
        "Error: configure as-path-list attributes and entry in separate "
        "commands (got as-path-list attribute '{}' alongside entry {})",
        listArgs.attr(),
        args.seqNum());
  }

  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool listCreated = !bgpcli::asPathListExists(cfg, listArgs.listName());
  auto& list = bgpcli::findOrCreateAsPathList(cfg, listArgs.listName());
  const bool entryCreated = !bgpcli::asPathListEntryExists(list, args.seqNum());
  auto& entry = bgpcli::findOrCreateAsPathListEntry(list, args.seqNum());

  Result result = args.attr().empty()
      ? ok(entryCreated
               ? fmt::format(
                     "Successfully created BGP as-path-list {} entry {}",
                     listArgs.listName(),
                     args.seqNum())
               : fmt::format(
                     "BGP as-path-list {} entry {} already exists",
                     listArgs.listName(),
                     args.seqNum()))
      // The attribute is guaranteed valid: BgpAsPathListEntryConfig's
      // constructor rejects an unknown attribute before we get here.
      : entryAttrHandlers().find(args.attr())->second(entry, args.values());

  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += fmt::format(
          " for as-path-list {} entry {}", listArgs.listName(), args.seqNum());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else {
    // A rejected value must not leave a phantom entry (or a phantom list
    // implicitly created for it) visible to later lookups in this process.
    if (entryCreated) {
      list.as_path_list()->pop_back();
    }
    if (listCreated) {
      cfg.policies()->aspath_lists()->pop_back();
    }
  }
  return result.message;
}

void CmdConfigProtocolBgpPolicyAsPathListEntry::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigProtocolBgpPolicyAsPathListEntry,
    CmdConfigProtocolBgpPolicyAsPathListEntryTraits>::run();

} // namespace facebook::fboss
