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
#include "fboss/cli/fboss2/CmdHandler.cpp"

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

std::string_view CmdShowPortQueueTraits::description() {
  return "Displays the egress queue configuration of each port: per queue, its ID, traffic class name, scheduling mode, weight, reserved bytes and dynamic threshold scaling factor. Weighted round robin queues report a weight while strict priority queues leave it blank. Use it to verify QoS queue and scheduler configuration on the ports.";
}

CmdShowPortQueue::RetType CmdShowPortQueue::sampleModel() {
  auto makeQueue = [](int32_t id,
                      const std::string& name,
                      const std::string& mode,
                      std::optional<int32_t> weight) {
    PortQueueThrift queue;
    queue.id() = id;
    queue.name() = name;
    queue.mode() = mode;
    if (weight.has_value()) {
      queue.weight() = *weight;
    }
    queue.reservedBytes() = 0;
    queue.scalingFactor() = "TWO";
    return queue;
  };

  std::vector<PortQueueThrift> queues = {
      makeQueue(0, "ncnf", "STRICT_PRIORITY", std::nullopt),
      makeQueue(1, "bronze", "STRICT_PRIORITY", std::nullopt),
      makeQueue(2, "silver", "STRICT_PRIORITY", std::nullopt),
      makeQueue(3, "gold", "STRICT_PRIORITY", std::nullopt),
      makeQueue(4, "", "WEIGHTED_ROUND_ROBIN", 1),
      makeQueue(5, "", "WEIGHTED_ROUND_ROBIN", 1),
      makeQueue(6, "icp", "STRICT_PRIORITY", std::nullopt),
      makeQueue(7, "nc", "STRICT_PRIORITY", std::nullopt),
  };

  RetType model;
  for (const auto& [portId, portName] :
       std::vector<std::pair<int32_t, std::string>>{
           {1, "eth1/2/1"}, {9, "eth1/1/1"}}) {
    PortInfoThrift portInfo;
    portInfo.portId() = portId;
    portInfo.name() = portName;
    portInfo.portQueues() = queues;
    model[portId] = portInfo;
  }
  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits>::getValidFilters();

} // namespace facebook::fboss
