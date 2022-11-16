// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonFileUtils.h"

using namespace ::testing;
using namespace facebook::fboss;

namespace {
constexpr int kMaxRetries{60};
constexpr int kL2AgeTimer{5};
constexpr int kL2LearnDelay{1};
} // unnamed namespace

class MacLearningTest : public LinkTest {
 private:
  void setupConfigFlag() override {
    XLOG(DBG2) << "setup up initial config for sw l2 learning mode, "
               << "since changing mode is not fully supported on SAI";
    overrideL2LearningConfig(true, kL2AgeTimer);
  }

 public:
  void updateL2Aging(int ageout) {
    sw()->updateStateBlocking("update L2 aging", [ageout](auto state) {
      auto newState = state->clone();
      auto switchSettings = newState->getSwitchSettings()->modify(&newState);
      switchSettings->setL2AgeTimerSeconds(ageout);
      return newState;
    });
  }

  void txPacket(MacAddress macAddr, VlanID vlan, PortID port) {
    auto txPacket = utility::makeEthTxPacket(
        sw()->getHw(),
        vlan,
        macAddr,
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);
    XLOG(DBG2) << "send packet with vlan " << vlan << " mac addr " << macAddr
               << " out of port " << port;
    sw()->getHw()->sendPacketOutOfPortSync(std::move(txPacket), port);
  }

  void verifyL2EntryLearned(MacAddress macAddr, VlanID vlanId) {
    auto l2EntryLearned = [&](const std::shared_ptr<SwitchState>& state) {
      auto vlan = state->getVlans()->getVlan(vlanId);
      auto* macTable = vlan->getMacTable().get();
      auto node = macTable->getMacIf(macAddr);
      XLOG(DBG2) << "found l2 entry node for " << macAddr << " and vlan "
                 << vlanId << ": " << (node != nullptr);
      return node != nullptr;
    };
    EXPECT_TRUE(waitForSwitchStateCondition(l2EntryLearned, kMaxRetries));
  }

  void verifyL2EntryValidated(PortID txPort, MacAddress srcMac) {
    // send packets whose src mac matches L2 entry and verify no drops,
    // if L2 entry is in pending state, these packets would be dropped
    auto ecmpPorts = getVlanOwningCabledPorts();
    programDefaultRoute(ecmpPorts, sw()->getPlatform()->getLocalMac());
    disableTTLDecrements(ecmpPorts);
    // wait long enough for all L2 entries learned/validated, port stats updated
    // sleep override
    sleep(5);
    // get discards before pump traffic
    auto portStats = sw()->getHw()->getPortStats();
    int maxDiscards = 0;
    for (auto [port, stats] : portStats) {
      auto inDiscards = *stats.inDiscards_();
      XLOG(DBG2) << "Port: " << port << " in discards: " << inDiscards
                 << " in bytes: " << *stats.inBytes_()
                 << " out bytes: " << *stats.outBytes_() << " at timestamp "
                 << *stats.timestamp_();
      if (inDiscards > maxDiscards) {
        maxDiscards = inDiscards;
      }
    }
    utility::pumpTraffic(
        true,
        sw()->getHw(),
        sw()->getPlatform()->getLocalMac(),
        sw()->getState()->getVlans()->getFirstVlanID(),
        txPort,
        255,
        srcMac);
    // no additional discards after pump traffic
    assertNoInDiscards(maxDiscards);
  }
};

TEST_F(MacLearningTest, l2EntryFlap) {
  auto verify = [this]() {
    auto macAddr = MacAddress("02:00:00:00:00:05");
    auto connectedPair = *getConnectedPairs().begin();
    auto txPort = connectedPair.first;
    auto rxPort = connectedPair.second;
    auto cfg = sw()->getConfig();
    auto portCfg = std::find_if(
        cfg.ports()->begin(), cfg.ports()->end(), [&rxPort](auto& p) {
          return PortID(*p.logicalID()) == rxPort;
        });
    auto vlan = VlanID(*portCfg->ingressVlan());
    txPacket(macAddr, vlan, txPort);
    verifyL2EntryLearned(macAddr, vlan);

    sw()->getUpdateEvb()->runInEventBaseThread([]() {
      XLOG(DBG2) << "Pause state update evb thread";
      // sleep override
      sleep(kL2AgeTimer + kL2LearnDelay);
    });
    // sleep override
    sleep(kL2AgeTimer);
    // previous learnt L2 entry should aged out by now

    txPacket(macAddr, vlan, txPort);
    // disable L2 aging
    updateL2Aging(0);

    // wait some time for all queued L2 entry state updates processed
    // sleep override
    sleep(kL2LearnDelay);

    // Send one more packet, L2 entry should be learned again
    txPacket(macAddr, vlan, txPort);

    verifyL2EntryLearned(macAddr, vlan);
    std::vector<L2EntryThrift> l2Entries;
    sw()->getHw()->fetchL2Table(&l2Entries);
    bool foundL2Entry = false;
    for (auto& l2Entry : l2Entries) {
      if (*l2Entry.mac() == macAddr.toString()) {
        XLOG(DBG2) << "L2 entry state is "
                   << (*l2Entry.l2EntryType() ==
                               L2EntryType::L2_ENTRY_TYPE_PENDING
                           ? "pending"
                           : "validated");
        foundL2Entry = true;
        EXPECT_TRUE(
            *l2Entry.l2EntryType() == L2EntryType::L2_ENTRY_TYPE_VALIDATED);
      }
    }
    EXPECT_TRUE(foundL2Entry);

    verifyL2EntryValidated(txPort, macAddr);
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
