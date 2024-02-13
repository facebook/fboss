/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"

#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>

#include <iostream>
#include <thread>

namespace facebook::fboss {

const std::string kDstIp = "2620:0:1cfe:face:b00c::4";

BENCHMARK(RxSlowPathBenchmark) {
  constexpr int kEcmpWidth = 1;
  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) {
        auto ports =
            ensemble.masterLogicalPortIds({cfg::PortType::INTERFACE_PORT});
        CHECK_GE(ports.size(), 1);

        // Before m-mpu agent test, use first Asic for initialization.
        auto switchIds = ensemble.getSw()->getHwAsicTable()->getSwitchIDs();
        CHECK_GE(switchIds.size(), 1);
        auto asic =
            ensemble.getSw()->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
        auto config =
            utility::onePortPerInterfaceConfig(ensemble.getSw(), {ports[0]});
        utility::addAclTableGroup(
            &config, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
        utility::addDefaultAclTable(config);
        // We don't want to set queue rate that limits the number of rx pkts
        utility::addCpuQueueConfig(
            config,
            asic,
            ensemble.isSai(),
            /* setQueueRate */ false);

        return config;
      };

  auto ensemble = createAgentEnsemble(initialConfigFn);

  // TODO(zecheng): Deprecate agent access to HwSwitch
  auto hwSwitch = ensemble->getHwSwitch();
  // capture packet exiting port 0 (entering due to loopback)
  auto trapDstIp = folly::CIDRNetwork{kDstIp, 128};
  auto packetCapture = HwTestPacketTrapEntry(hwSwitch, trapDstIp);
  auto dstMac = utility::getFirstInterfaceMac(ensemble->getProgrammedState());
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getProgrammedState(), dstMac);
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, kEcmpWidth);
  });
  ecmpHelper.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(
          ensemble->getSw(), ensemble->getSw()->getRib()),
      kEcmpWidth);
  // Disable TTL decrements
  utility::ttlDecrementHandlingForLoopbackTraffic(
      hwSwitch, ecmpHelper.getRouterId(), ecmpHelper.getNextHops()[0]);

  const auto kSrcMac = folly::MacAddress{"fa:ce:b0:00:00:0c"};
  // Send packet
  auto vlanId = utility::firstVlanID(ensemble->getProgrammedState());
  auto txPacket = utility::makeUDPTxPacket(
      hwSwitch,
      vlanId,
      kSrcMac,
      dstMac,
      folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
      folly::IPAddressV6(kDstIp),
      8000,
      8001);
  hwSwitch->sendPacketSwitchedSync(std::move(txPacket));

  constexpr auto kBurnIntevalInSeconds = 5;
  // Let the packet flood warm up
  std::this_thread::sleep_for(std::chrono::seconds(kBurnIntevalInSeconds));
  constexpr uint8_t kCpuQueue = 0;
  auto [pktsBefore, bytesBefore] =
      utility::getCpuQueueOutPacketsAndBytes(hwSwitch, kCpuQueue);
  auto timeBefore = std::chrono::steady_clock::now();
  CHECK_NE(pktsBefore, 0);
  std::this_thread::sleep_for(std::chrono::seconds(kBurnIntevalInSeconds));
  auto [pktsAfter, bytesAfter] =
      utility::getCpuQueueOutPacketsAndBytes(hwSwitch, kCpuQueue);
  auto timeAfter = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> durationMillseconds =
      timeAfter - timeBefore;
  uint32_t pps = (static_cast<double>(pktsAfter - pktsBefore) /
                  durationMillseconds.count()) *
      1000;
  uint32_t bytesPerSec = (static_cast<double>(bytesAfter - bytesBefore) /
                          durationMillseconds.count()) *
      1000;

  if (FLAGS_json) {
    folly::dynamic cpuRxRateJson = folly::dynamic::object;
    cpuRxRateJson["cpu_rx_pps"] = pps;
    cpuRxRateJson["cpu_rx_bytes_per_sec"] = bytesPerSec;
    std::cout << toPrettyJson(cpuRxRateJson) << std::endl;
  } else {
    XLOG(DBG2) << " Pkts before: " << pktsBefore << " Pkts after: " << pktsAfter
               << " interval ms: " << durationMillseconds.count()
               << " pps: " << pps << " bytes per sec: " << bytesPerSec;
  }
}
} // namespace facebook::fboss
