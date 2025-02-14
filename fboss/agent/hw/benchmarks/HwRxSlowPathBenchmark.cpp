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
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>

#include <iostream>
#include <thread>

namespace facebook::fboss {

const std::string kDstIp = "2620:0:1cfe:face:b00c::4";

BENCHMARK(RxSlowPathBenchmark) {
  AgentEnsembleSwitchConfigFn initialConfigFn = [](const AgentEnsemble&
                                                       ensemble) {
    FLAGS_sai_user_defined_trap = true;
    CHECK_GE(
        ensemble.masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}).size(),
        1);
    std::vector<PortID> ports = {ensemble.masterLogicalInterfacePortIds()[0]};

    // For J2 and J3, initialize recycle port as well to allow l3 lookup on
    // recycle port
    if (ensemble.getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::CPU_TX_VIA_RECYCLE_PORT)) {
      ports.push_back(
          ensemble.masterLogicalPortIds({cfg::PortType::RECYCLE_PORT})[0]);
    }
    auto config = utility::onePortPerInterfaceConfig(ensemble.getSw(), ports);
    utility::setupDefaultAclTableGroups(config);

    // We don't want to set queue rate that limits the number of rx pkts
    utility::addCpuQueueConfig(
        config,
        ensemble.getL3Asics(),
        ensemble.isSai(),
        /* setQueueRate */ false);
    auto trapDstIp = folly::CIDRNetwork{kDstIp, 128};
    auto asic = utility::checkSameAndGetAsic(ensemble.getL3Asics());
    utility::addTrapPacketAcl(asic, &config, trapDstIp);

    // Since J2 and J3 does not support disabling TLL on port, create TRAP to
    // forward TTL=0 packet. Also not send icmp time exceeded packet, since CPU
    // will trap TTL=0 packet.
    if (ensemble.getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::CPU_TX_VIA_RECYCLE_PORT)) {
      FLAGS_send_icmp_time_exceeded = false;
      utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    }
    return config;
  };

  auto ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);

  // capture packet exiting port 0 (entering due to loopback)
  auto dstMac = utility::getFirstInterfaceMac(ensemble->getProgrammedState());
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getProgrammedState(), dstMac);
  flat_set<PortDescriptor> firstIntfPort;
  firstIntfPort.insert(
      PortDescriptor(ensemble->masterLogicalInterfacePortIds()[0]));
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, firstIntfPort);
  });
  ecmpHelper.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(
          ensemble->getSw(), ensemble->getSw()->getRib()),
      firstIntfPort,
      {RoutePrefixV6{folly::IPAddressV6(), 0}});
  // Disable TTL decrements
  utility::ttlDecrementHandlingForLoopbackTraffic(
      ensemble.get(), ecmpHelper.getRouterId(), ecmpHelper.getNextHops()[0]);

  const auto kSrcMac = folly::MacAddress{"fa:ce:b0:00:00:0c"};
  // Send packet
  auto vlanId = utility::firstVlanID(ensemble->getProgrammedState());
  auto constexpr kPacketToSend = 10;
  for (int i = 0; i < kPacketToSend; i++) {
    auto txPacket = utility::makeUDPTxPacket(
        ensemble->getSw(),
        vlanId,
        kSrcMac,
        dstMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6(kDstIp),
        8000,
        8001);
    ensemble->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
  }

  constexpr auto kBurnIntevalInSeconds = 10;
  // Let the packet flood warm up
  std::this_thread::sleep_for(std::chrono::seconds(kBurnIntevalInSeconds));
  constexpr uint8_t kCpuQueue = 0;
  std::map<int, CpuPortStats> cpuStatsBefore;
  ensemble->getSw()->getAllCpuPortStats(cpuStatsBefore);
  auto statsBefore = cpuStatsBefore[0];
  auto [pktsBefore, bytesBefore] = utility::getCpuQueueOutPacketsAndBytes(
      *statsBefore.portStats_(), kCpuQueue);
  auto timeBefore = std::chrono::steady_clock::now();
  CHECK_NE(pktsBefore, 0);
  std::this_thread::sleep_for(std::chrono::seconds(kBurnIntevalInSeconds));
  std::map<int, CpuPortStats> cpuStatsAfter;
  ensemble->getSw()->getAllCpuPortStats(cpuStatsAfter);
  auto statsAfter = cpuStatsAfter[0];
  auto [pktsAfter, bytesAfter] = utility::getCpuQueueOutPacketsAndBytes(
      *statsAfter.portStats_(), kCpuQueue);
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
