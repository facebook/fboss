/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwOverflowTest.h"

#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/ProdConfigFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <chrono>
namespace {
constexpr auto kTxRxThresholdMs = 2000;
}

namespace facebook::fboss {

cfg::SwitchConfig HwOverflowTest::initialConfig() const {
  // Since we run inavriant tests based on switch role, we just pick the
  // most common switch role (i.e. rsw) for convenience of testing.
  // Not intended to extend coverage for every platform
  auto config =
      utility::createProdRswConfig(getHwSwitch(), masterLogicalPortIds());
  return config;
}
void HwOverflowTest::SetUp() {
  HwLinkStateDependentTest::SetUp();
  prodInvariants_ = std::make_unique<HwProdInvariantHelper>(
      getHwSwitchEnsemble(), initialConfig());
  prodInvariants_->setupEcmp();
}

void HwOverflowTest::startPacketTxRxVerify() {
  CHECK(!packetRxVerifyRunning_);
  packetRxVerifyRunning_ = true;
  auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
  auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
  auto dstIp = folly::IPAddress::createNetwork(
                   initialConfig().interfaces_ref()[0].ipAddresses_ref()[0])
                   .first;
  auto sendBgpPktToMe = [=]() {
    utility::sendTcpPkts(
        getHwSwitch(),
        1 /*numPktsToSend*/,
        vlanId,
        intfMac,
        dstIp,
        utility::kNonSpecialPort1,
        utility::kBgpPort,
        masterLogicalPortIds()[0]);
  };
  packetRxVerifyThread_ =
      std::make_unique<std::thread>([this, sendBgpPktToMe]() {
        HwTestPacketSnooper snooper(getHwSwitchEnsemble());
        initThread("PacketRxTxVerify");
        while (packetRxVerifyRunning_) {
          auto startTime(std::chrono::steady_clock::now());
          sendBgpPktToMe();
          snooper.waitForPacket();
          std::chrono::duration<double, std::milli> durationMillseconds =
              std::chrono::steady_clock::now() - startTime;
          if (durationMillseconds.count() > kTxRxThresholdMs) {
            throw FbossError(
                "Took more than : ", kTxRxThresholdMs, " for TX, RX cycle");
          }
        }
      });
}

void HwOverflowTest::stopPacketTxRxVerify() {
  packetRxVerifyRunning_ = false;
  packetRxVerifyThread_->join();
  packetRxVerifyThread_.reset();
  // Wait slightly more than  kTxRxThresholdMs  so as to drain any inflight
  // traffic from packetTxRxThread send loop. Else the infligh traffic can
  // effect subsequent verifications by bumping up counters
  sleep(kTxRxThresholdMs / 1000 + 1);
}
} // namespace facebook::fboss
