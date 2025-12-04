/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/InvariantTestUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/validated_shell_commands_constants.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"

namespace facebook::fboss::utility {

void verifyCopp(SwSwitch* sw, SwitchID switchId, PortID port) {
  utility::verifyCoppInvariantHelper(
      sw,
      switchId,
      sw->getHwAsicTable()->getHwAsicIf(switchId),
      sw->getState(),
      port);
}

void verifySafeDiagCmds(TestEnsembleIf* ensemble) {
  std::set<std::string> diagCmds;
  for (const auto& [switchId, asic] :
       ensemble->getHwAsicTable()->getHwAsics()) {
    switch (asic->getAsicType()) {
      case cfg::AsicType::ASIC_TYPE_FAKE:
      case cfg::AsicType::ASIC_TYPE_MOCK:
      case cfg::AsicType::ASIC_TYPE_EBRO:
      case cfg::AsicType::ASIC_TYPE_GARONNE:
      case cfg::AsicType::ASIC_TYPE_YUBA:
      case cfg::AsicType::ASIC_TYPE_CHENAB:
      case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
      case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
      case cfg::AsicType::ASIC_TYPE_AGERA3:
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
      case cfg::AsicType::ASIC_TYPE_RAMON:
      case cfg::AsicType::ASIC_TYPE_RAMON3:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK6:
        break;

      case cfg::AsicType::ASIC_TYPE_TRIDENT2:
        diagCmds = validated_shell_commands_constants::TD2_TESTED_CMDS();
        break;
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
        diagCmds = validated_shell_commands_constants::TH_TESTED_CMDS();
        break;
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
        diagCmds = validated_shell_commands_constants::TH3_TESTED_CMDS();
        break;
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
        diagCmds = validated_shell_commands_constants::TH4_TESTED_CMDS();
        break;
    }
    if (diagCmds.size()) {
      for (auto i = 0; i < 10; ++i) {
        for (const auto& cmd : diagCmds) {
          std::string out;
          ensemble->runDiagCommand(cmd + "\n", out, switchId);
        }
      }
      std::string out;
      ensemble->runDiagCommand("quit\n", out, switchId);
    }
  }
}

void verifyLoadBalance(
    SwSwitch* sw,
    int ecmpWidth,
    const std::vector<PortDescriptor>& ecmpPorts) {
  std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
      getPortStatsFn = [&](const std::vector<PortID>& portIds) {
        return sw->getHwPortStats(portIds);
      };
  utility::pumpTrafficAndVerifyLoadBalanced(
      [=]() {
        auto intfMac =
            utility::getMacForFirstInterfaceWithPorts(sw->getState());
        utility::pumpTraffic(
            true,
            utility::getAllocatePktFn(sw),
            utility::getSendPktFunc(sw),
            intfMac,
            sw->getState()->getVlans()->getFirstVlanID());
      },
      [=]() {
        auto ports = std::make_unique<std::vector<int32_t>>();
        for (const auto& ecmpPort : ecmpPorts) {
          ports->push_back(static_cast<int32_t>(ecmpPort.phyPortID()));
        }
        sw->clearPortStats(ports);
      },
      [=]() {
        return utility::isLoadBalanced(
            ecmpPorts,
            std::vector<NextHopWeight>(ecmpWidth, 1),
            getPortStatsFn,
            25);
      });
}

void verifyDscpToQueueMapping(SwSwitch* sw, const std::vector<PortID>& ports) {
  if (!sw->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::L3_QOS)) {
    return;
  }
  // lambda that returns HwPortStats for the given port
  auto getPortStats = [&]() { return sw->getHwPortStats(ports); };

  auto q2dscpMap = utility::getOlympicQosMaps(sw->getConfig());
  EXPECT_TRUE(
      utility::verifyQueueMappingsInvariantEcmpHelper(
          q2dscpMap, sw, sw->getState(), getPortStats, ports));
}

} // namespace facebook::fboss::utility
