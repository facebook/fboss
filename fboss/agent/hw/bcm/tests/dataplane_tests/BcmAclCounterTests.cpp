/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/tests/BcmAclUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddressV6.h>

using folly::IPAddressV6;

namespace facebook::fboss {

class BcmAclCounterTest : public BcmLinkStateDependentTests {
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
    constexpr auto kAclName = "ttld-acl";
    auto acl = utility::addAcl(config, kAclName);
    acl->srcIp_ref().value_unchecked() = "2001::/64";
    acl->__isset.srcIp = true;
    acl->proto_ref().value_unchecked() = 0x11;
    acl->__isset.proto = true;
    acl->ipType_ref().value_unchecked() = cfg::IpType::IP6;
    acl->__isset.ipType = true;
    acl->ttl_ref().value_unchecked().value = 128;
    acl->ttl_ref().value_unchecked().mask = 128;
    acl->__isset.ttl = true;
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

TEST_F(BcmAclCounterTest, TestCounterBumpOnHit) {
  auto setup = [this]() { setupAclStat(); };
  auto verify = [this]() {
    auto statHandle =
        getHwSwitch()->getAclTable()->getAclStat(kCounterName)->getHandle();
    auto statBefore = utility::getAclInOutPackets(getUnit(), statHandle);
    sendPkt(200);
    auto statAfter = utility::getAclInOutPackets(getUnit(), statHandle);
    // Bump by 2, once on the way out and once when it loops back in
    EXPECT_EQ(statBefore + 2, statAfter);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclCounterTest, TestCounterNoHitNoBump) {
  auto setup = [this]() { setupAclStat(); };
  auto verify = [=]() {
    auto statHandle =
        getHwSwitch()->getAclTable()->getAclStat(kCounterName)->getHandle();
    auto statBefore = utility::getAclInOutPackets(getUnit(), statHandle);
    sendPkt(10);
    auto statAfter = utility::getAclInOutPackets(getUnit(), statHandle);
    EXPECT_EQ(statBefore, statAfter);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
