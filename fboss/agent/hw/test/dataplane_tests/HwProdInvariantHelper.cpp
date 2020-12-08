/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwProdInvariantHelper.h"

#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace {
auto constexpr kEcmpWidth = 4;
}
namespace facebook::fboss {

void HwOverflowTest::setupEcmp() {
  ecmpHelper_ = std::make_unique<utility::HwIpV6EcmpDataPlaneTestUtil>(
      getHwSwitchEnsemble(), RouterID(0));
  ecmpHelper_->setupECMPForwarding(
      kEcmpWidth, std::vector<NextHopWeight>(kEcmpWidth, 1));
}

void HwOverflowTest::verifyLoadBalacing() {
  CHECK(ecmpHelper_);
  ecmpHelper_->pumpTrafficThroughPort(masterLogicalPortIds()[kEcmpWidth]);
  ecmpHelper_->isLoadBalanced(
      kEcmpWidth, std::vector<NextHopWeight>(kEcmpWidth, 1), 25);
}

void HwOverflowTest::verifyCopp() {
  auto sendPkts = [this] {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto dstIp = folly::IPAddress::createNetwork(
                     initialConfig().interfaces_ref()[0].ipAddresses_ref()[0])
                     .first;
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
  utility::sendPktAndVerifyCpuQueue(
      getHwSwitch(), utility::getCoppHighPriQueueId(getAsic()), sendPkts, 1);
}

void HwOverflowTest::verifyDscpToQueueMapping() {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }
  auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
  auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
  auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
  for (const auto& q2dscps : utility::kOlympicQueueToDscp()) {
    for (auto dscp : q2dscps.second) {
      utility::sendTcpPkts(
          getHwSwitch(),
          1 /*numPktsToSend*/,
          vlanId,
          intfMac,
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
          8000,
          8001,
          masterLogicalPortIds()[kEcmpWidth],
          dscp);
    }
  }
  bool mappingVerified = false;
  for (auto i = 0; i < kEcmpWidth; ++i) {
    // Since we don't know which port the above IP will get hashed to,
    // iterate over all ports in ecmp group to find one which satisfies
    // dscp to queue mapping.
    if (mappingVerified) {
      break;
    }
    auto portId =
        ecmpHelper_->ecmpSetupHelper()->ecmpPortDescriptorAt(i).phyPortID();
    mappingVerified = utility::verifyQueueMappings(
        portStatsBefore[portId],
        utility::kOlympicQueueToDscp(),
        getHwSwitchEnsemble(),
        portId);
  }
  EXPECT_TRUE(mappingVerified);
}
} // namespace facebook::fboss
