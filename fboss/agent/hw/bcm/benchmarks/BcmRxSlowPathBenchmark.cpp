/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmControlPlaneQueueManager.h"

#include <folly/IPAddressV6.h>
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>
#include "common/time/Time.h"
#include "fboss/agent/hw/bcm/BcmError.h"

#include <iostream>

extern "C" {
#include <bcm/cosq.h>
}

DEFINE_bool(json, true, "Output in json form");

namespace facebook::fboss {

std::pair<uint64_t, uint64_t> getCpuQueueStats(
    const BcmSwitch* bcmSwitch,
    bcm_cos_queue_t cosq) {
  // TODO: Have BcmControlPlaneManger save last collected
  // stats and retrieve these from it.
  auto controlPlaneQueueMgr = bcmSwitch->getControlPlane()->getQueueManager();
  bcm_gport_t gport = bcmSwitch->getControlPlane()->getCPUGPort();
  if (bcmSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::L3_QOS)) {
    std::tie(gport, cosq) = std::make_pair(
        controlPlaneQueueMgr->getQueueGPort(cfg::StreamType::MULTICAST, cosq),
        0);
  }
  uint64_t pkts, bytes;
  auto rv = bcm_cosq_stat_get(
      bcmSwitch->getUnit(), gport, cosq, bcmCosqStatOutPackets, &pkts);
  bcmCheckError(rv, "Couldn't collect cpu pkt stats");
  rv = bcm_cosq_stat_get(
      bcmSwitch->getUnit(), gport, cosq, bcmCosqStatOutBytes, &bytes);
  bcmCheckError(rv, "Couldn't collect cpu pkt stats");
  return std::make_pair(pkts, bytes);
}

void runRxSlowPathBenchmark() {
  constexpr int kEcmpWidth = 1;
  BcmSwitchEnsemble ensemble;
  auto bcmSwitch = ensemble.getHwSwitch();
  auto config = utility::oneL3IntfConfig(
      bcmSwitch, ensemble.getPlatform()->masterLogicalPortIds()[0]);
  ensemble.applyInitialConfig(config);
  // capture packet exiting port 0 (entering due to loopback)
  auto packetCapture = HwTestPacketTrapEntry(
      bcmSwitch, ensemble.getPlatform()->masterLogicalPortIds()[0]);
  auto cpuMac = bcmSwitch->getPlatform()->getLocalMac();
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble.getProgrammedState(), cpuMac);
  auto ecmpRouteState = ecmpHelper.setupECMPForwarding(
      ecmpHelper.resolveNextHops(ensemble.getProgrammedState(), kEcmpWidth),
      kEcmpWidth);
  ensemble.applyNewState(ecmpRouteState);
  // Disable TTL decrements
  utility::disableTTLDecrements(
      bcmSwitch,
      ecmpHelper.getRouterId(),
      folly::IPAddress(ecmpHelper.getNextHops()[0].ip));

  // Send packet
  auto txPacket = utility::makeUDPTxPacket(
      bcmSwitch,
      VlanID(*config.vlanPorts[0].vlanID_ref()),
      cpuMac,
      cpuMac,
      folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
      folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
      8000,
      8001);
  bcmSwitch->sendPacketSwitchedSync(std::move(txPacket));

  constexpr bcm_cos_queue_t kCpuQueue = 1;
  constexpr auto kBurnIntevalMs = 5000;
  // Let the packet flood warm up
  WallClockMs::Burn(kBurnIntevalMs);

  auto [pktsBefore, bytesBefore] = getCpuQueueStats(bcmSwitch, kCpuQueue);
  auto timeBefore = WallClockMs::Now();
  CHECK_NE(pktsBefore, 0);
  WallClockMs::Burn(kBurnIntevalMs);

  auto [pktsAfter, bytesAfter] = getCpuQueueStats(bcmSwitch, kCpuQueue);
  auto timeAfter = WallClockMs::Now();
  uint32_t pps =
      (static_cast<double>(pktsAfter - pktsBefore) / (timeAfter - timeBefore)) *
      1000;
  uint32_t bytesPerSec = (static_cast<double>(bytesAfter - bytesBefore) /
                          (timeAfter - timeBefore)) *
      1000;

  if (FLAGS_json) {
    folly::dynamic cpuRxRateJson = folly::dynamic::object;
    cpuRxRateJson["cpu_rx_pps"] = pps;
    cpuRxRateJson["cpu_rx_bytes_per_sec"] = bytesPerSec;
    std::cout << toPrettyJson(cpuRxRateJson) << std::endl;
  } else {
    XLOG(INFO) << " Pkts before: " << pktsBefore << " Pkts after: " << pktsAfter
               << " interval ms: " << timeAfter - timeBefore << " pps: " << pps
               << " bytes per sec: " << bytesPerSec;
  }
}
} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  facebook::fboss::runRxSlowPathBenchmark();
  return 0;
}
