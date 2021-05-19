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

#include "fboss/agent/hw/test/gen-cpp2/validated_shell_commands_constants.h"

namespace {
auto constexpr kEcmpWidth = 4;
}
namespace facebook::fboss {

void HwProdInvariantHelper::setupEcmp() {
  ecmpHelper_ = std::make_unique<utility::HwIpV6EcmpDataPlaneTestUtil>(
      ensemble_, RouterID(0));
  ecmpHelper_->programRoutes(
      kEcmpWidth, std::vector<NextHopWeight>(kEcmpWidth, 1));
}

void HwProdInvariantHelper::verifyLoadBalacing() {
  CHECK(ecmpHelper_);
  ecmpHelper_->pumpTrafficThroughPort(
      ensemble_->masterLogicalPortIds()[kEcmpWidth]);
  ecmpHelper_->isLoadBalanced(
      kEcmpWidth, std::vector<NextHopWeight>(kEcmpWidth, 1), 25);
}

void HwProdInvariantHelper::verifyCopp() {
  auto sendPkts = [this] {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac =
        utility::getInterfaceMac(ensemble_->getProgrammedState(), vlanId);
    auto dstIp = folly::IPAddress::createNetwork(
                     initialConfig().interfaces_ref()[0].ipAddresses_ref()[0])
                     .first;
    utility::sendTcpPkts(
        ensemble_->getHwSwitch(),
        1 /*numPktsToSend*/,
        vlanId,
        intfMac,
        dstIp,
        utility::kNonSpecialPort1,
        utility::kBgpPort,
        ensemble_->masterLogicalPortIds()[0]);
  };
  utility::sendPktAndVerifyCpuQueue(
      ensemble_->getHwSwitch(),
      utility::getCoppHighPriQueueId(ensemble_->getPlatform()->getAsic()),
      sendPkts,
      1);
}

void HwProdInvariantHelper::verifyDscpToQueueMapping() {
  if (!ensemble_->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }
  auto portStatsBefore =
      ensemble_->getLatestPortStats(ensemble_->masterLogicalPortIds());
  auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
  auto intfMac =
      utility::getInterfaceMac(ensemble_->getProgrammedState(), vlanId);
  for (const auto& q2dscps : utility::kOlympicQueueToDscp()) {
    for (auto dscp : q2dscps.second) {
      utility::sendTcpPkts(
          ensemble_->getHwSwitch(),
          1 /*numPktsToSend*/,
          vlanId,
          intfMac,
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
          8000,
          8001,
          ensemble_->masterLogicalPortIds()[kEcmpWidth],
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
        ensemble_,
        portId);
  }
  EXPECT_TRUE(mappingVerified);
}

void HwProdInvariantHelper::verifySafeDiagCmds() {
  std::set<std::string> diagCmds;
  switch (ensemble_->getAsic()->getAsicType()) {
    case HwAsic::AsicType::ASIC_TYPE_FAKE:
    case HwAsic::AsicType::ASIC_TYPE_MOCK:
    case HwAsic::AsicType::ASIC_TYPE_TAJO:
    case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
      break;

    case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
      diagCmds = validated_shell_commands_constants::TD2_TESTED_CMDS();
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
      diagCmds = validated_shell_commands_constants::TH_TESTED_CMDS();
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      diagCmds = validated_shell_commands_constants::TH3_TESTED_CMDS();
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
      diagCmds = validated_shell_commands_constants::TH4_TESTED_CMDS();
      break;
  }
  if (diagCmds.size()) {
    for (auto i = 0; i < 10; ++i) {
      for (auto cmd : diagCmds) {
        std::string out;
        ensemble_->runDiagCommand(cmd + "\n", out);
      }
    }
    std::string out;
    ensemble_->runDiagCommand("quit\n", out);
  }
}

} // namespace facebook::fboss
