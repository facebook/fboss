/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/IPAddress.h>
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
constexpr uint8_t kDefaultQueue = 0;
constexpr uint8_t kTestingQueue = 7;
constexpr uint32_t kDscp = 0x24;
} // namespace

namespace facebook::fboss {

class AgentSendPacketToQueueTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_FORWARDING};
  }

 protected:
  void checkSendPacket(std::optional<uint8_t> ucQueue, bool isOutOfPort);
};

void AgentSendPacketToQueueTest::checkSendPacket(
    std::optional<uint8_t> ucQueue,
    bool isOutOfPort) {
  auto setup = [=, this]() {
    if (!isOutOfPort) {
      // need to set up ecmp for switching
      auto kEcmpWidthForTest = 1;
      utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
      resolveNeigborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
    }
  };

  auto verify = [=, this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
    auto port = ecmpHelper6.nhop(0).portDesc.phyPortID();
    const uint8_t queueID = ucQueue ? *ucQueue : kDefaultQueue;

    auto beforeOutPkts =
        getLatestPortStats(port).get_queueOutPackets_().at(queueID);
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    // packet format shouldn't be matter in this test
    auto pkt = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001);

    if (isOutOfPort) {
      if (ucQueue) {
        getAgentEnsemble()->ensureSendPacketOutOfPort(
            std::move(pkt), port, *ucQueue);
      } else {
        getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), port);
      }
    } else {
      getAgentEnsemble()->ensureSendPacketSwitched(std::move(pkt));
    }

    WITH_RETRIES({
      auto afterOutPkts =
          getLatestPortStats(port).get_queueOutPackets_().at(queueID);

      /*
       * Once the packet egresses out of the asic, the packet will be looped
       * back with dmac as neighbor mac. This will certainly fail the my mac
       * check. Some asic vendors drop the packet right away in the pipeline
       * whereas some drop later in the pipeline after MMU once the packet is
       * queueed. This will cause the queue counters to increment more than
       * once. Always check if atleast 1 packet is received.
       */
      EXPECT_EVENTUALLY_GE(afterOutPkts - beforeOutPkts, 1);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentSendPacketToQueueTest, SendPacketOutOfPortToUCQueue) {
  checkSendPacket(kTestingQueue, true);
}

TEST_F(AgentSendPacketToQueueTest, SendPacketOutOfPortToDefaultUCQueue) {
  checkSendPacket(std::nullopt, true);
}

TEST_F(AgentSendPacketToQueueTest, SendPacketSwitchedToDefaultUCQueue) {
  checkSendPacket(std::nullopt, false);
}

class AgentSendPacketToMulticastQueueTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::L3_FORWARDING,
        production_features::ProductionFeature::MULTICAST_QUEUE};
  }
};

TEST_F(AgentSendPacketToMulticastQueueTest, SendPacketOutOfPortToMCQueue) {
  auto ensemble = getAgentEnsemble();
  auto l3Asics = ensemble->getSw()->getHwAsicTable()->getL3Asics();
  auto asic = utility::checkSameAndGetAsic(l3Asics);
  auto masterLogicalPortIds = ensemble->masterLogicalPortIds();
  auto port = masterLogicalPortIds[0];

  auto setup = [=, this]() {
    // put all ports in the same vlan
    auto newCfg = utility::oneL3IntfNPortConfig(
        ensemble->getSw()->getPlatformMapping(),
        asic,
        masterLogicalPortIds,
        ensemble->getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    applyNewConfig(newCfg);
  };

  auto verifyFlooding = [=, this]() {
    auto beforeOutPkts = *getLatestPortStats(port).outMulticastPkts_();
    WITH_RETRIES({
      auto afterOutPkts = *getLatestPortStats(port).outMulticastPkts_();
      XLOG(DBG2) << "afterOutPkts " << afterOutPkts << ", beforeOutPkts "
                 << beforeOutPkts;
      EXPECT_EVENTUALLY_GT(afterOutPkts - beforeOutPkts, 10000);
    });
  };

  auto verify = [=, this]() {
    if (getSw()->getBootType() == BootType::WARM_BOOT) {
      return;
    }
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto randomMac = folly::MacAddress("01:02:03:04:05:06");
    // send packets with random dst mac to flood the vlan
    for (int i = 0; i < 100; i++) {
      auto pkt = utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          intfMac,
          randomMac,
          folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
          8000,
          8001,
          kDscp << 2);
      ensemble->sendPacketAsync(
          std::move(pkt),
          PortDescriptor(masterLogicalPortIds[0]),
          std::nullopt);
    }

    XLOG(DBG2) << "Verify multicast traffic is flooding";
    verifyFlooding();

    XLOG(DBG2)
        << "Add ACL to send packet to queue 7. This should only affect unicast traffic but not multicast traffic here";
    // In S422033, multicast traffic was also send to multicast queue 7, which
    // does not exist on TH4 and corrupted the asic. This caused parity error
    // later. All packets are also dropped and no traffic flooding any more.
    auto newCfg = ensemble->getCurrentConfig();
    utility::addDscpAclToCfg(asic, &newCfg, "acl1", kDscp);
    utility::addQueueMatcher(&newCfg, "acl1", kTestingQueue, ensemble->isSai());
    applyNewConfig(newCfg);

    XLOG(DBG2) << "Wait 10 seconds and verify multicast traffic still flooding";
    sleep(10);
    verifyFlooding();
  };

  auto verifyPostWb = [&]() {
    XLOG(DBG2) << "Verify multicast traffic still flooding after warmboot";
    verifyFlooding();
  };
  verifyAcrossWarmBoots(setup, verify, []() {}, verifyPostWb);
}

} // namespace facebook::fboss
