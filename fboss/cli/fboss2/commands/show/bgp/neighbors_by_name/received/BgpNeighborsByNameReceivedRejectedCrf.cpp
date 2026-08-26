/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/neighbors_by_name/received/BgpNeighborsByNameReceivedRejectedCrf.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

BgpNeighborsByNameReceivedRejectedCrf::RetType
BgpNeighborsByNameReceivedRejectedCrf::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& regexPatterns,
    const std::vector<std::string>& prefixes) {
  if (regexPatterns.empty()) {
    std::cout
        << "This command requires a regex pattern as argument: "
        << "fboss2 show bgp neighbors-by-name <REGEX_PATTERN> received rejected crf"
        << std::endl;
    return {};
  }
  return BgpNeighborsByNameReceivedRejected::queryRejectedPrefixes(
      hostInfo, regexPatterns[0], prefixes, /*crfOnly=*/true);
}

void BgpNeighborsByNameReceivedRejectedCrf::printOutput(
    const RetType& result,
    std::ostream& out) {
  BgpNeighborsByNameAdvertisedRejected::printRejectedResult(result, out);
}

template void CmdHandler<
    BgpNeighborsByNameReceivedRejectedCrf,
    BgpNeighborsByNameReceivedRejectedCrfTraits>::run();

} // namespace facebook::fboss
