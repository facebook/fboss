/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"
#include "fboss/thrift_cow/visitors/PathVisitor.h"

#include <fb303/ServiceData.h>
#include <folly/String.h>

namespace facebook::fboss::utility {

std::string getCurrentStateJSONForPathHelper(
    const std::string& path,
    const std::shared_ptr<SwitchState>& sw);

void clearSwPortStats(
    std::vector<int32_t>& ports,
    std::shared_ptr<SwitchState> state);

template <typename SwitchT>
void clearPortStats(
    SwitchT* sw,
    std::unique_ptr<std::vector<int32_t>> ports,
    std::shared_ptr<SwitchState> state) {
  sw->clearPortStats(ports);

  auto getPortCounterKeys = [&](std::vector<std::string>& portKeys,
                                const folly::StringPiece prefix,
                                const std::shared_ptr<Port> port) {
    auto portId = port->getID();
    auto portName = port->getName().empty()
        ? folly::to<std::string>("port", portId)
        : port->getName();
    auto portNameWithPrefix = folly::to<std::string>(portName, ".", prefix);
    portKeys.emplace_back(
        folly::to<std::string>(portNameWithPrefix, "unicast_pkts"));
    portKeys.emplace_back(folly::to<std::string>(portNameWithPrefix, "bytes"));
    portKeys.emplace_back(
        folly::to<std::string>(portNameWithPrefix, "multicast_pkts"));
    portKeys.emplace_back(
        folly::to<std::string>(portNameWithPrefix, "broadcast_pkts"));
    portKeys.emplace_back(folly::to<std::string>(portNameWithPrefix, "errors"));
    portKeys.emplace_back(
        folly::to<std::string>(portNameWithPrefix, "discards"));
  };

  auto getQueueCounterKeys = [&](std::vector<std::string>& portKeys,
                                 const std::shared_ptr<Port> port) {
    auto portId = port->getID();
    auto portName = port->getName().empty()
        ? folly::to<std::string>("port", portId)
        : port->getName();
    for (int i = 0; i < port->getPortQueues()->size(); ++i) {
      auto portQueue = folly::to<std::string>(portName, ".", "queue", i, ".");
      portKeys.emplace_back(
          folly::to<std::string>(portQueue, "out_congestion_discards_bytes"));
      portKeys.emplace_back(folly::to<std::string>(portQueue, "out_bytes"));
    }
  };

  auto getPortPfcCounterKeys = [&](std::vector<std::string>& portKeys,
                                   const std::shared_ptr<Port> port) {
    auto portId = port->getID();
    auto portName = port->getName().empty()
        ? folly::to<std::string>("port", portId)
        : port->getName();
    auto portNameExt = folly::to<std::string>(portName, ".");
    std::array<int, 2> enabledPfcPriorities_{0, 7};
    for (auto pri : enabledPfcPriorities_) {
      portKeys.emplace_back(
          folly::to<std::string>(portNameExt, "in_pfc_frames.priority", pri));
      portKeys.emplace_back(
          folly::to<std::string>(
              portNameExt, "in_pfc_xon_frames.priority", pri));
      portKeys.emplace_back(
          folly::to<std::string>(portNameExt, "out_pfc_frames.priority", pri));
    }
    portKeys.emplace_back(folly::to<std::string>(portNameExt, "in_pfc_frames"));
    portKeys.emplace_back(
        folly::to<std::string>(portNameExt, "out_pfc_frames"));
  };

  auto statsMap = facebook::fb303::fbData->getStatMap();
  for (const auto& portId : *ports) {
    const auto port = state->getPorts()->getNodeIf(PortID(portId));
    std::vector<std::string> portKeys;
    getPortCounterKeys(portKeys, "out_", port);
    getPortCounterKeys(portKeys, "in_", port);
    getQueueCounterKeys(portKeys, port);
    if (port->getPfc().has_value()) {
      getPortPfcCounterKeys(portKeys, port);
    }
    for (const auto& key : portKeys) {
      // this API locks statistics for the key
      // ensuring no race condition with update/delete
      // in different thread
      statsMap->clearValue(key);
    }
  }
}
} // namespace facebook::fboss::utility
