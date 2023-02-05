/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddressV6.h>
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>

#include <chrono>
#include <iostream>
#include <thread>

DEFINE_bool(json, true, "Output in json form");
DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set to true will prepare the device for warmboot");

namespace facebook::fboss {

std::pair<uint64_t, uint64_t> getOutPktsAndBytes(
    HwSwitchEnsemble* ensemble,
    PortID port) {
  auto stats = ensemble->getLatestPortStats(port);
  return {*stats.outUnicastPkts_(), *stats.outBytes_()};
}

void runTxSlowPathBenchmark() {
  constexpr int kEcmpWidth = 1;
  auto ensemble = createAndInitHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  auto hwSwitch = ensemble->getHwSwitch();
  auto portUsed = ensemble->masterLogicalPortIds()[0];
  auto config = utility::oneL3IntfConfig(hwSwitch, portUsed);
  ensemble->applyInitialConfig(config);
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getProgrammedState());

  ensemble->applyNewState(
      ecmpHelper.resolveNextHops(ensemble->getProgrammedState(), kEcmpWidth));
  ecmpHelper.programRoutes(
      std::make_unique<HwSwitchEnsembleRouteUpdateWrapper>(
          ensemble->getRouteUpdater()),
      kEcmpWidth);
  auto cpuMac = ensemble->getPlatform()->getLocalMac();
  std::atomic<bool> packetTxDone{false};
  std::thread t([cpuMac, hwSwitch, &config, &packetTxDone]() {
    const auto kSrcIp = folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
    const auto kDstIp = folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
    const auto kSrcMac = folly::MacAddress{"fa:ce:b0:00:00:0c"};
    while (!packetTxDone) {
      for (auto i = 0; i < 1'000; ++i) {
        // Send packet
        auto txPacket = utility::makeIpTxPacket(
            hwSwitch,
            VlanID(*config.vlanPorts()[0].vlanID()),
            kSrcMac,
            cpuMac,
            kSrcIp,
            kDstIp);
        hwSwitch->sendPacketSwitchedAsync(std::move(txPacket));
      }
    }
  });

  auto [pktsBefore, bytesBefore] =
      getOutPktsAndBytes(ensemble.get(), PortID(portUsed));
  auto timeBefore = std::chrono::steady_clock::now();
  // Let the packet flood warm up
  std::this_thread::sleep_for(std::chrono::seconds(5));
  auto [pktsAfter, bytesAfter] =
      getOutPktsAndBytes(ensemble.get(), PortID(portUsed));
  auto timeAfter = std::chrono::steady_clock::now();
  packetTxDone = true;
  t.join();
  std::chrono::duration<double, std::milli> durationMillseconds =
      timeAfter - timeBefore;
  uint32_t pps = (static_cast<double>(pktsAfter - pktsBefore) /
                  durationMillseconds.count()) *
      1000;
  uint32_t bytesPerSec = (static_cast<double>(bytesAfter - bytesBefore) /
                          durationMillseconds.count()) *
      1000;

  if (FLAGS_json) {
    folly::dynamic cpuTxRateJson = folly::dynamic::object;
    cpuTxRateJson["cpu_tx_pps"] = pps;
    cpuTxRateJson["cpu_tx_bytes_per_sec"] = bytesPerSec;
    std::cout << toPrettyJson(cpuTxRateJson) << std::endl;
  } else {
    XLOG(DBG2) << " Pkts before: " << pktsBefore << " Pkts after: " << pktsAfter
               << " interval ms: " << durationMillseconds.count()
               << " pps: " << pps << " bytes per sec: " << bytesPerSec;
  }
}
} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  facebook::fboss::runTxSlowPathBenchmark();
  return 0;
}
