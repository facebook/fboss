/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowPortQueue.h"

#include <fmt/core.h>

namespace facebook::fboss {

CmdShowPortQueue::RetType CmdShowPortQueue::queryClient(
    const HostInfo& hostInfo,
    const utils::PortList& queriedPorts) {
  RetType portEntries;

  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getAllPortInfo(portEntries);

  if (queriedPorts.size() == 0) {
    return portEntries;
  }

  RetType retVal;
  for (auto const& [portId, portInfo] : portEntries) {
    for (auto const& queriedPort : queriedPorts.data()) {
      if (portInfo.name().value() == queriedPort) {
        retVal.insert(std::make_pair(portId, portInfo));
      }
    }
  }

  return retVal;
}

void CmdShowPortQueue::printOutput(
    const RetType& portId2PortInfoThrift,
    std::ostream& out) {
  constexpr auto fmtString = "{:<7}{:<20}{:<25}{:<10}{:<15}{:<15}\n";

  for (auto const& [portId, portInfo] : portId2PortInfoThrift) {
    std::ignore = portId;

    out << portInfo.name().value() << "\n";
    out << std::string(10, '=') << std::endl;

    out << fmt::format(
        fmtString,
        "ID",
        "Name",
        "Mode",
        "Weight",
        "ReservedBytes",
        "ScalingFactor");
    out << std::string(90, '-') << std::endl;

    for (auto const& queue : portInfo.portQueues().value()) {
      out << fmt::format(
          fmtString,
          folly::copy(queue.id().value()),
          queue.name().value(),
          queue.mode().value(),
          queue.weight() ? std::to_string(*queue.weight()) : "",
          queue.reservedBytes() ? std::to_string(*queue.reservedBytes()) : "",
          queue.scalingFactor() ? *queue.scalingFactor() : "");
    }

    out << "\n";
  }
}

} // namespace facebook::fboss
