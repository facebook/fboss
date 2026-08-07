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
#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliAttrHandlers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/BgpAsPathListCliUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {

// The attribute names, exactly as documented. Kept here so the
// valid-attribute set and the handler table stay in sync. The list's entries
// are their own subcommand, not attributes.
constexpr std::string_view kDescription = "description";

using AsPathList = bgp::bgp_policy::AsPathList;
using bgpcli::AttrHandler;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::Result;

// ---- setters ----------------------------------------------------------------
// Each writes one already-parsed, already-validated value. Parsing and message
// text belong to the shared factories in BgpCliAttrHandlers.h.

void setListDescription(AsPathList& list, const std::string& description) {
  list.description() = description;
}

// ---- list-level attribute registry ------------------------------------------
// One line per documented attribute: its dispatch key, its value shape, and
// the setter that stores it.
const std::map<std::string, AttrHandler<AsPathList>, std::less<>>&
listAttrHandlers() {
  static const std::map<std::string, AttrHandler<AsPathList>, std::less<>>
      kHandlers = {
          {std::string(kDescription),
           joinedStringAttr<AsPathList>(kDescription, setListDescription)},
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
// errors (same mechanism as BgpPeerGroupConfig).
BgpAsPathListConfig::BgpAsPathListConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: as-path-list <name> is required, optionally followed by an "
        "<attribute> <value>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument("Error: as-path-list name must not be empty");
  }
  listName_ = v[0];

  if (v.size() == 1) {
    return; // bare `as-path-list <name>`: create the list
  }

  attr_ = v[1];
  values_.assign(v.begin() + 2, v.end());

  if (listAttrHandlers().find(attr_) == listAttrHandlers().end()) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unknown as-path-list attribute '{}'. Valid attributes: {}",
            attr_,
            validAttrList()));
  }
}

CmdConfigProtocolBgpPolicyAsPathListTraits::RetType
CmdConfigProtocolBgpPolicyAsPathList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool listCreated = !bgpcli::asPathListExists(cfg, args.listName());
  auto& list = bgpcli::findOrCreateAsPathList(cfg, args.listName());

  Result result = args.attr().empty()
      ? ok(listCreated
               ? fmt::format(
                     "Successfully created BGP as-path-list {}",
                     args.listName())
               : fmt::format(
                     "BGP as-path-list {} already exists", args.listName()))
      // The attribute is guaranteed valid: BgpAsPathListConfig's constructor
      // rejects an unknown attribute before we get here.
      : listAttrHandlers().find(args.attr())->second(list, args.values());

  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += fmt::format(" for as-path-list {}", args.listName());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else if (listCreated) {
    // Drop the phantom list so a rejected value is not visible to later
    // lookups in the same process.
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
