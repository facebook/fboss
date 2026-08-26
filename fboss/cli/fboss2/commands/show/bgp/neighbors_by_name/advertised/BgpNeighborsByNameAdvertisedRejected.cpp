/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/neighbors_by_name/advertised/BgpNeighborsByNameAdvertisedRejected.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

BgpNeighborsByNameAdvertisedRejected::RetType
BgpNeighborsByNameAdvertisedRejected::queryRejectedPrefixes(
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

  std::cout << "Querying rejected advertised prefixes for "
            << matchingSessions.size() << " matching neighbor(s)..."
            << std::endl;

  for (const auto& session : matchingSessions) {
    const auto& peerAddr = *session.peer_addr();
    const auto& description = *session.description();

    std::map<TIpPrefix, std::vector<TBgpPath>> preFilterNetworks;
    std::map<TIpPrefix, std::vector<TBgpPath>> postFilterNetworks;

    client->sync_getPrefilterAdvertisedNetworks2(preFilterNetworks, peerAddr);
    client->sync_getPostfilterAdvertisedNetworks2(postFilterNetworks, peerAddr);

    auto rejectedNetworks =
        getRejectedNetworks(preFilterNetworks, postFilterNetworks);

    if (!prefixes.empty()) {
      rejectedNetworks = filterNetworks(rejectedNetworks, prefixes);
    }

    for (const auto& [ipPrefix, paths] : rejectedNetworks) {
      auto prefixStr = ipPrefixToString(ipPrefix);
      for (const auto& path : paths) {
        auto policyName = getPolicyName(path);
        if (crfOnly && !isCrfRejection(policyName)) {
          continue;
        }
        auto key = makeKey(prefixStr, policyName);
        result[key].push_back(fmt::format("{} ({})", peerAddr, description));
      }
    }
  }

  return result;
}

BgpNeighborsByNameAdvertisedRejected::RetType
BgpNeighborsByNameAdvertisedRejected::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& regexPatterns,
    const ObjectArgType& prefixes) {
  if (regexPatterns.empty()) {
    std::cout
        << "This command requires a regex pattern as argument: "
        << "fboss2 show bgp neighbors-by-name <REGEX_PATTERN> advertised rejected"
        << std::endl;
    return {};
  }
  return queryRejectedPrefixes(
      hostInfo, regexPatterns[0], prefixes, /*crfOnly=*/false);
}

void BgpNeighborsByNameAdvertisedRejected::printRejectedResult(
    const RetType& result,
    std::ostream& out) {
  if (result.empty()) {
    out << "No rejected prefixes found." << std::endl;
    return;
  }

  out << fmt::format("{} rejected prefix/policy group(s)", result.size())
      << std::endl;
  out << std::endl;

  for (const auto& [key, neighbors] : result) {
    auto sepPos = key.find(" | ");
    if (sepPos == std::string::npos) {
      out << fmt::format("Prefix: {}", key) << std::endl;
      out << fmt::format("  Neighbor(s) ({}):", neighbors.size()) << std::endl;
      for (const auto& neighbor : neighbors) {
        out << fmt::format("    {}", neighbor) << std::endl;
      }
      out << std::endl;
      continue;
    }
    auto prefix = key.substr(0, sepPos);
    auto policy = key.substr(sepPos + 3);
    out << fmt::format("Prefix: {}", prefix) << std::endl;
    out << fmt::format("  Rejected reason: {}", policy) << std::endl;
    out << fmt::format("  Neighbor(s) ({}):", neighbors.size()) << std::endl;
    for (const auto& neighbor : neighbors) {
      out << fmt::format("    {}", neighbor) << std::endl;
    }
    out << std::endl;
  }
}

void BgpNeighborsByNameAdvertisedRejected::printOutput(
    const RetType& result,
    std::ostream& out) {
  printRejectedResult(result, out);
}

template void CmdHandler<
    BgpNeighborsByNameAdvertisedRejected,
    BgpNeighborsByNameAdvertisedRejectedTraits>::run();

} // namespace facebook::fboss
