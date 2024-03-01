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
#include "fboss/lib/CommonUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thread>

using namespace facebook::fboss;

namespace facebook::fboss::utility {
namespace {

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const PortID portId,
    uint32_t sleep = 200) {
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
      XLOG(DBG2) << "queue " << _q2dscps.first << " queuePacketsBefore "
                 << queuePacketsBefore << " queuePacketsAfter "
                 << queuePacketsAfter;
      if (queuePacketsAfter < queuePacketsBefore + _q2dscps.second.size()) {
        statsMatch = false;
        break;
      }
    }
    if (!statsMatch) {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    } else {
      break;
    }
    XLOG(DBG2) << " Retrying ...";
  } while (--retries && !statsMatch);
  return statsMatch;
}
bool queueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort) {
  auto queuePacketsBefore =
      portStatsBefore.queueOutPackets_()->find(queueId)->second;
  auto portStatsAfter = ensemble->getLatestPortStats(egressPort);
  auto queuePacketsAfter = portStatsAfter.queueOutPackets_()[queueId];
  // Note, on some platforms, due to how loopbacked packets are pruned
  // from being broadcast, they will appear more than once on a queue
  // counter, so we can only check that the counter went up, not that it
  // went up by exactly one.
  XLOG(DBG2) << "Port ID: " << egressPort << " queue: " << queueId
             << " queuePacketsBefore " << queuePacketsBefore
             << " queuePacketsAfter " << queuePacketsAfter;
  return queuePacketsAfter > queuePacketsBefore;
}

bool voqHit(
    const HwSysPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::SystemPortID egressPort) {
  auto queueBytesBefore =
      portStatsBefore.queueOutBytes_()->find(queueId)->second;
  auto portStatsAfter = ensemble->getLatestSysPortStats(egressPort);
  auto queueBytesAfter = portStatsAfter.queueOutBytes_()[queueId];
  XLOG(DBG2) << "Sys port: " << egressPort << " queue " << queueId
             << " queueBytesBefore " << queueBytesBefore << " queueBytesAfter "
             << queueBytesAfter;
  return queueBytesAfter > queueBytesBefore;
}
} // namespace

void verifyQueueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort) {
  WITH_RETRIES({
    EXPECT_EVENTUALLY_TRUE(
        queueHit(portStatsBefore, queueId, ensemble, egressPort));
  });
}

void verifyVoQHit(
    const HwSysPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::SystemPortID egressPort) {
  WITH_RETRIES({
    EXPECT_EVENTUALLY_TRUE(
        voqHit(portStatsBefore, queueId, ensemble, egressPort));
  });
}

bool verifyQueueMappingsInvariantHelper(
    const std::map<int, std::vector<uint8_t>>& q2dscpMap,
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const std::vector<PortID>& ecmpPorts,
    uint32_t sleep) {
  auto portStatsBefore = getAllHwPortStats();
  auto vlanId = utility::firstVlanID(swState);
  auto intfMac = utility::getFirstInterfaceMac(swState);
  auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);

  for (const auto& q2dscps : q2dscpMap) {
    for (auto dscp : q2dscps.second) {
      auto pkt = makeTCPTxPacket(
          hwSwitch,
          vlanId,
          srcMac,
          intfMac,
          folly::IPAddressV6("1::10"),
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
          8000,
          8001,
          dscp << 2);
      hwSwitch->sendPacketSwitchedSync(std::move(pkt));
    }
  }

  bool mappingVerified = false;
  for (auto& ecmpPort : ecmpPorts) {
    // Since we don't know which port the above IP will get hashed to,
    // iterate over all ports in ecmp group to find one which satisfies
    // dscp to queue mapping.
    if (mappingVerified) {
      XLOG(DBG2) << "Mapping verified!";
      break;
    }

    XLOG(DBG2) << "Verifying mapping for ecmp port: " << (int)ecmpPort;
    mappingVerified = verifyQueueMappings(
        portStatsBefore[ecmpPort],
        q2dscpMap,
        getAllHwPortStats,
        ecmpPort,
        sleep);
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
