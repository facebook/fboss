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
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddressV6.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

using folly::IPAddressV6;

namespace facebook::fboss {

class HwAclCounterTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  void sendPkt(uint8_t ttl) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto pkt = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        IPAddressV6("2001::1"),
        IPAddressV6("2800::1"),
        10000,
        20000,
        0,
        ttl);
    getHwSwitch()->sendPacketSwitchedSync(std::move(pkt));
  }

  static constexpr auto kAclName = "ttld-acl";
  static constexpr auto kCounterName = "ttld-stat";

  void setupAclStat() {
    auto initialCfg = initialConfig();
    addTtlAclStat(&initialCfg);
    applyNewConfig(initialCfg);
    ecmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), kRid);
    setupECMPForwarding();
  }

 private:
  void addTtlAclStat(cfg::SwitchConfig* config) const {
    auto acl = utility::addAcl(config, kAclName);
    acl->srcIp_ref() = "2001::/64";
    acl->proto_ref() = 0x11;
    acl->ipType_ref() = cfg::IpType::IP6;
    acl->ttl_ref() = cfg::Ttl();
    *acl->ttl_ref()->value_ref() = 128;
    *acl->ttl_ref()->mask_ref() = 128;
    utility::addAclStat(config, kAclName, kCounterName);
  }
  void setupECMPForwarding() {
    auto constexpr kEcmpWidth = 1;
    auto newState = ecmpHelper_->setupECMPForwarding(
        ecmpHelper_->resolveNextHops(getProgrammedState(), kEcmpWidth),
        kEcmpWidth);
    applyNewState(newState);
  }

  const RouterID kRid{0};
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> ecmpHelper_;
};

TEST_F(HwAclCounterTest, TestCounterBumpOnHit) {
  auto setup = [this]() { setupAclStat(); };
  auto verify = [this]() {
    auto statBefore = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), kAclName, kCounterName);
    sendPkt(200);
    auto statAfter = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), kAclName, kCounterName);
    // Bump by 2, once on the way out and once when it loops back in
    EXPECT_EQ(statBefore + 2, statAfter);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclCounterTest, TestCounterNoHitNoBump) {
  auto setup = [this]() { setupAclStat(); };
  auto verify = [=]() {
    auto statBefore = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), kAclName, kCounterName);
    sendPkt(10);
    auto statAfter = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), kAclName, kCounterName);
    EXPECT_EQ(statBefore, statAfter);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
