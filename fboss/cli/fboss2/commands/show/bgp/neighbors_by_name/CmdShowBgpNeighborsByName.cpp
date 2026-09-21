/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/neighbors_by_name/CmdShowBgpNeighborsByName.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

std::vector<TBgpSession> CmdShowBgpNeighborsByName::filterNeighborsByNameRegex(
    const std::vector<TBgpSession>& allSessions,
    const std::string& pattern) {
  re2::RE2 regex(pattern);
  if (!regex.ok()) {
    std::cerr << "Invalid regex pattern: " << pattern << " (" << regex.error()
              << ")" << std::endl;
    return {};
  }
  std::vector<TBgpSession> filtered;
  for (const auto& session : allSessions) {
    if (re2::RE2::PartialMatch(*session.description(), regex)) {
      filtered.push_back(session);
    }
  }
  return filtered;
}

CmdShowBgpNeighborsByName::RetType CmdShowBgpNeighborsByName::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& regexPatterns) {
  if (regexPatterns.empty()) {
    std::cout << "This command requires a regex pattern as argument: "
              << "fboss2 show bgp neighbors-by-name <REGEX_PATTERN>"
              << std::endl;
    return {};
  }

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  std::vector<TBgpSession> allSessions;
  client->sync_getBgpNeighbors(allSessions, {});

  return filterNeighborsByNameRegex(allSessions, regexPatterns[0]);
}

void CmdShowBgpNeighborsByName::printOutput(
    const RetType& neighbors,
    std::ostream& out) {
  if (neighbors.empty()) {
    out << "No neighbors matched the given pattern." << std::endl;
    return;
  }
  printBgpNeighborsOutput(neighbors, out);
}

template void
CmdHandler<CmdShowBgpNeighborsByName, CmdShowBgpNeighborsByNameTraits>::run();

} // namespace facebook::fboss
