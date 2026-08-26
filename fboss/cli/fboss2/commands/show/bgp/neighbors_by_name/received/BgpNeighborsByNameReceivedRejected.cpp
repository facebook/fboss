/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/neighbors_by_name/received/BgpNeighborsByNameReceivedRejected.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

BgpNeighborsByNameReceivedRejected::RetType
BgpNeighborsByNameReceivedRejected::queryRejectedPrefixes(
    const HostInfo& hostInfo,
    const std::string& regexPattern,
    const ObjectArgType& prefixes,
    bool crfOnly) {
  RetType result;

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  std::vector<facebook::neteng::fboss::bgp::thrift::TBgpSession> allSessions;
  client->sync_getBgpNeighbors(allSessions, {});

  auto matchingSessions = CmdShowBgpNeighborsByName::filterNeighborsByNameRegex(
      allSessions, regexPattern);

  if (matchingSessions.empty()) {
    std::cout << "No neighbors matched pattern: " << regexPattern << std::endl;
    return result;
  }

  std::cout << "Querying rejected received prefixes for "
            << matchingSessions.size() << " matching neighbor(s)..."
            << std::endl;

  for (const auto& session : matchingSessions) {
    const auto& peerAddr = *session.peer_addr();
    const auto& description = *session.description();

    std::map<TIpPrefix, std::vector<TBgpPath>> preFilterNetworks;
    std::map<TIpPrefix, std::vector<TBgpPath>> postFilterNetworks;

    client->sync_getPrefilterReceivedNetworks2(preFilterNetworks, peerAddr);
    client->sync_getPostfilterReceivedNetworks2(postFilterNetworks, peerAddr);

    auto rejectedNetworks =
        getRejectedNetworks(preFilterNetworks, postFilterNetworks);

    if (!prefixes.empty()) {
      rejectedNetworks = filterNetworks(rejectedNetworks, prefixes);
    }

    for (const auto& [ipPrefix, paths] : rejectedNetworks) {
      auto prefixStr =
          BgpNeighborsByNameAdvertisedRejected::ipPrefixToString(ipPrefix);
      for (const auto& path : paths) {
        auto policyName =
            BgpNeighborsByNameAdvertisedRejected::getPolicyName(path);
        if (crfOnly &&
            !BgpNeighborsByNameAdvertisedRejected::isCrfRejection(policyName)) {
          continue;
        }
        auto key = BgpNeighborsByNameAdvertisedRejected::makeKey(
            prefixStr, policyName);
        result[key].push_back(fmt::format("{} ({})", peerAddr, description));
      }
    }
  }

  return result;
}

BgpNeighborsByNameReceivedRejected::RetType
BgpNeighborsByNameReceivedRejected::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& regexPatterns,
    const ObjectArgType& prefixes) {
  if (regexPatterns.empty()) {
    std::cout
        << "This command requires a regex pattern as argument: "
        << "fboss2 show bgp neighbors-by-name <REGEX_PATTERN> received rejected"
        << std::endl;
    return {};
  }
  return queryRejectedPrefixes(
      hostInfo, regexPatterns[0], prefixes, /*crfOnly=*/false);
}

void BgpNeighborsByNameReceivedRejected::printOutput(
    const RetType& result,
    std::ostream& out) {
  BgpNeighborsByNameAdvertisedRejected::printRejectedResult(result, out);
}

template void CmdHandler<
    BgpNeighborsByNameReceivedRejected,
    BgpNeighborsByNameReceivedRejectedTraits>::run();

} // namespace facebook::fboss
