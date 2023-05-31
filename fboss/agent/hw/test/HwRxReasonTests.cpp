/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"

namespace facebook::fboss {

class HwRxReasonTests : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true /*interfaceHasSubnet*/);

    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    // Remove DHCP from rxReason list
    auto rxReasonListWithoutDHCP = utility::getCoppRxReasonToQueues(getAsic());
    auto dhcpRxReason = ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::DHCP, utility::kCoppMidPriQueueId);
    auto dhcpRxReasonIter = std::find(
        rxReasonListWithoutDHCP.begin(),
        rxReasonListWithoutDHCP.end(),
        dhcpRxReason);
    if (dhcpRxReasonIter != rxReasonListWithoutDHCP.end()) {
      rxReasonListWithoutDHCP.erase(dhcpRxReasonIter);
    }
    cfg.cpuTrafficPolicy()->rxReasonToQueueOrderedList() =
        rxReasonListWithoutDHCP;
    return cfg;
  }
};

TEST_F(HwRxReasonTests, InsertAndRemoveRxReason) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto cfg = initialConfig();
    applyNewConfig(cfg);
    cfg.cpuTrafficPolicy()->rxReasonToQueueOrderedList() =
        utility::getCoppRxReasonToQueues(getAsic());
    applyNewConfig(cfg);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
