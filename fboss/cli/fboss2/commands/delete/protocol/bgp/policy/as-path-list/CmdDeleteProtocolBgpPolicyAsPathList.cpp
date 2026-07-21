/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/as-path-list/CmdDeleteProtocolBgpPolicyAsPathList.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <algorithm>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

// Parse + validate at construction so queryClient stays a thin dispatch.
BgpAsPathListRef::BgpAsPathListRef(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.size() != 1) {
    throw std::invalid_argument(
        "Error: delete protocol bgp policy as-path-list requires exactly one "
        "<name>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument("Error: as-path-list name must not be empty");
  }
  listName_ = std::move(v[0]);
}

CmdDeleteProtocolBgpPolicyAsPathListTraits::RetType
CmdDeleteProtocolBgpPolicyAsPathList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  if (!cfg.policies().has_value()) {
    // Nothing is persisted for an unknown list, so a typo'd delete can't stage
    // an unrelated session change.
    return fmt::format("Error: BGP as-path-list {} not found", args.listName());
  }
  auto& lists = *cfg.policies()->aspath_lists();
  auto it = std::find_if(lists.begin(), lists.end(), [&](const auto& list) {
    return *list.name() == args.listName();
  });
  if (it == lists.end()) {
    return fmt::format("Error: BGP as-path-list {} not found", args.listName());
  }
  lists.erase(it);
  session.saveBgpConfig();
  return fmt::format(
      "Successfully deleted BGP as-path-list {}\nConfig saved to: {}",
      args.listName(),
      session.getBgpSessionConfigPath());
}

void CmdDeleteProtocolBgpPolicyAsPathList::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdDeleteProtocolBgpPolicyAsPathList,
    CmdDeleteProtocolBgpPolicyAsPathListTraits>::run();

} // namespace facebook::fboss
