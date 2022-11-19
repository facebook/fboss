/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace facebook::fboss;

namespace facebook::fboss::utility {

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const PortID portId) {
  auto retries = 10;
  bool statsMatch;
  do {
    auto portStatsAfter = getAllHwPortStats();
    statsMatch = true;
    for (const auto& _q2dscps : q2dscps) {
      auto queuePacketsBefore =
          portStatsBefore.queueOutPackets_()->find(_q2dscps.first)->second;
      auto queuePacketsAfter =
          portStatsAfter[portId].queueOutPackets_()[_q2dscps.first];
      // Note, on some platforms, due to how loopbacked packets are pruned
      // from being broadcast, they will appear more than once on a queue
      // counter, so we can only check that the counter went up, not that it
      // went up by exactly one.
      if (queuePacketsAfter < queuePacketsBefore + _q2dscps.second.size()) {
        statsMatch = false;
        break;
      }
    }
    if (!statsMatch) {
      std::this_thread::sleep_for(20ms);
    } else {
      break;
    }
    XLOG(DBG2) << " Retrying ...";
  } while (--retries && !statsMatch);
  return statsMatch;
}

bool verifyQueueMappingsInvariantHelper(
    const std::map<int, std::vector<uint8_t>>& q2dscpMap,
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const std::vector<PortID>& ecmpPorts,
    PortID portId) {
  auto portStatsBefore = getAllHwPortStats();
  auto vlanId = utility::firstVlanID(swState);
  auto intfMac = utility::getFirstInterfaceMac(swState);

  for (const auto& q2dscps : q2dscpMap) {
    for (auto dscp : q2dscps.second) {
      utility::sendTcpPkts(
          hwSwitch,
          1 /*numPktsToSend*/,
          vlanId,
          intfMac,
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
          8000,
          8001,
          portId,
          dscp);
    }
  }

  bool mappingVerified = false;
  for (auto& ecmpPort : ecmpPorts) {
    // Since we don't know which port the above IP will get hashed to,
    // iterate over all ports in ecmp group to find one which satisfies
    // dscp to queue mapping.
    if (mappingVerified) {
      break;
    }
    XLOG(DBG2) << "Mapping verified for : " << (int)ecmpPort;

    mappingVerified = verifyQueueMappings(
        portStatsBefore[ecmpPort], q2dscpMap, getAllHwPortStats, ecmpPort);
  }
  return mappingVerified;
}

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort) {
  // lambda that returns HwPortStats for the given port
  auto getPortStats = [ensemble,
                       egressPort]() -> std::map<PortID, HwPortStats> {
    std::vector<facebook::fboss::PortID> portIds = {egressPort};
    return ensemble->getLatestPortStats(portIds);
  };

  return verifyQueueMappings(
      portStatsBefore, q2dscps, getPortStats, egressPort);
}
} // namespace facebook::fboss::utility
