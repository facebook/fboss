// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/Utils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/TrunkTestUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace facebook::fboss {
class AgentTrunkTest : public AgentHwTest {
 protected:
  AggregatePortID kAggPort1() const {
    return AggregatePortID(1);
  }
  AggregatePortID kAggPortMax() const {
    return AggregatePortID(std::numeric_limits<AggregatePortID>::max());
  }
  std::vector<PortID> getAggPortMembers(
      AggregatePortID aggPort,
      const AgentEnsemble& ensemble) const {
    auto ports = ensemble.masterLogicalPortIds();
    if (aggPort == kAggPort1()) {
      return {ports[0], ports[1]};
    }
    CHECK(aggPort == kAggPortMax());
    return {ports[2], ports[3]};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto asic = checkSameAndGetAsicForTesting(ensemble.getL3Asics());
    auto ports = ensemble.masterLogicalPortIds();
    return utility::oneAggregatePortPerInterfaceConfig(
        ensemble.getPlatformMapping(),
        asic,
        {ports[0], ports[1], ports[2], ports[3]},
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes(),
        {{kAggPort1(), getAggPortMembers(kAggPort1(), ensemble)},
         {kAggPortMax(), getAggPortMembers(kAggPortMax(), ensemble)}},
        cfg::InterfaceType::VLAN);
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::LAG};
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& config) {
    applyNewConfig(config);
    applyNewState(
        [](const std::shared_ptr<SwitchState> state) {
          return utility::enableTrunkPorts(state);
        },
        "enable trunk ports");
  }
};

TEST_F(AgentTrunkTest, TrunkCreateHighLowKeyIds) {
  auto setup = [=, this]() {
    applyConfigAndEnableTrunks(initialConfig(*getAgentEnsemble()));
  };
  auto verify = [=, this]() {
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          utility::getAggregatePortCount(*getAgentEnsemble()), 2);
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePort(*getAgentEnsemble(), kAggPort1()));
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePort(*getAgentEnsemble(), kAggPortMax()));
      auto aggIDs = {kAggPort1(), kAggPortMax()};
      for (auto aggId : aggIDs) {
        EXPECT_EVENTUALLY_TRUE(
            utility::verifyAggregatePortMemberCount(
                *getAgentEnsemble(), aggId, 2));
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTrunkTest, TrunkCheckIngressPktAggPort) {
  auto setup = [=, this]() {
    applyConfigAndEnableTrunks(initialConfig(*getAgentEnsemble()));
    for (auto member : getAggPortMembers(kAggPort1(), *getAgentEnsemble())) {
      bringDownPort(member);
    }
  };
  auto verify = [=, this]() {
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyPktFromAggregatePort(
              *getAgentEnsemble(), kAggPortMax()));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTrunkTest, TrunkMemberPortDownMinLinksViolated) {
  auto setup = [=, this]() {
    applyConfigAndEnableTrunks(initialConfig(*getAgentEnsemble()));
    for (auto member : getAggPortMembers(kAggPort1(), *getAgentEnsemble())) {
      bringDownPort(member);
    }
  };
  auto verify = [=, this]() {
    // Member port count should drop to 1 now.
    bringDownPort(getAggPortMembers(kAggPortMax(), *getAgentEnsemble())[0]);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          utility::getAggregatePortCount(*getAgentEnsemble()), 2);
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePort(*getAgentEnsemble(), kAggPortMax()));
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePortMemberCount(
              *getAgentEnsemble(), kAggPortMax(), 1));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTrunkTest, TrunkPortStats) {
  auto setup = [=, this]() {
    applyConfigAndEnableTrunks(initialConfig(*getAgentEnsemble()));
    auto ecmpHelper = utility::EcmpSetupTargetedPorts6(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          return ecmpHelper.resolveNextHops(in, {PortDescriptor(kAggPort1())});
        },
        "resolve next hops");
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&routeUpdater, {PortDescriptor(kAggPort1())});
    bringDownPort(getAggPortMembers(kAggPortMax(), *getAgentEnsemble())[0]);
  };
  auto verify = [=, this]() {
    const std::string kTrunkName = "AGG-1";
    for (auto throughPort : {false, true}) {
      // Tag for the ingress port's own interface: out-of-port injection on
      // Agg-max member, the trunk member's interface for switched packets.
      auto injectPort = throughPort
          ? getAggPortMembers(kAggPortMax(), *getAgentEnsemble())[1]
          : getAggPortMembers(kAggPort1(), *getAgentEnsemble())[1];
      auto state = getProgrammedState();
      auto intf = state->getInterfaces()->getNodeIf(
          getInterfaceIDForPort(injectPort, state));
      CHECK(intf != nullptr);
      auto vlanId = getSw()->getVlanIDForTx(intf);
      auto intfMac = intf->getMac();
      auto portPkts0 = int64_t{0};
      for (auto member : getAggPortMembers(kAggPort1(), *getAgentEnsemble())) {
        auto portStats = getLatestPortStats(member);
        portPkts0 += *portStats.outUnicastPkts_() +
            *portStats.outMulticastPkts_() + *portStats.outBroadcastPkts_();
      }

      auto getTrunkOutPkts = [&](const std::string& trunkName) -> int64_t {
        auto hwStats = getAllHwSwitchStats();
        for (const auto& [_, switchStats] : hwStats) {
          auto it = switchStats.hwTrunkStats()->find(trunkName);
          if (it != switchStats.hwTrunkStats()->end()) {
            return *it->second.outUnicastPkts_() +
                *it->second.outMulticastPkts_() +
                *it->second.outBroadcastPkts_();
          }
        }
        return 0;
      };

      int64_t trunkPkts0 = getTrunkOutPkts(kTrunkName);

      auto pkt = utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          intfMac,
          intfMac,
          folly::IPAddress("2401::1"),
          folly::IPAddress("2401::2"),
          10001,
          20001);
      throughPort
          ? getAgentEnsemble()->ensureSendPacketOutOfPort(
                std::move(pkt),
                getAggPortMembers(kAggPortMax(), *getAgentEnsemble())[1])
          : getAgentEnsemble()->ensureSendPacketSwitched(std::move(pkt));

      auto portPkts1 = int64_t{0};
      for (auto member : getAggPortMembers(kAggPort1(), *getAgentEnsemble())) {
        auto portStats = getLatestPortStats(member);
        portPkts1 += *portStats.outUnicastPkts_() +
            *portStats.outMulticastPkts_() + *portStats.outBroadcastPkts_();
      }

      int64_t trunkPkts1 = getTrunkOutPkts(kTrunkName);

      EXPECT_GT(portPkts1, portPkts0);
      EXPECT_GT(trunkPkts1, trunkPkts0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
TEST_F(AgentTrunkTest, TrunkCapacityUpdatesOnMemberDown) {
  auto setup = [=, this]() {
    applyConfigAndEnableTrunks(initialConfig(*getAgentEnsemble()));
    for (auto member : getAggPortMembers(kAggPortMax(), *getAgentEnsemble())) {
      bringDownPort(member);
    }
  };
  auto verify = [=, this]() {
    const std::string kTrunkName = "AGG-1";

    auto getTrunkCapacity = [&]() -> int64_t {
      auto hwStats = getAllHwSwitchStats();
      for (const auto& [_, switchStats] : hwStats) {
        auto it = switchStats.hwTrunkStats()->find(kTrunkName);
        if (it != switchStats.hwTrunkStats()->end()) {
          return *it->second.capacity_();
        }
      }
      return -1;
    };

    auto state = getProgrammedState();
    auto member0Speed = static_cast<int64_t>(
        state->getPorts()
            ->getNodeIf(getAggPortMembers(kAggPort1(), *getAgentEnsemble())[0])
            ->getSpeed());
    auto member1Speed = static_cast<int64_t>(
        state->getPorts()
            ->getNodeIf(getAggPortMembers(kAggPort1(), *getAgentEnsemble())[1])
            ->getSpeed());

    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(getTrunkCapacity(), member0Speed + member1Speed);
    });

    bringDownPort(getAggPortMembers(kAggPort1(), *getAgentEnsemble())[0]);

    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(getTrunkCapacity(), member1Speed); });
    this->applyNewState(
        [](const std::shared_ptr<SwitchState> state) {
          return utility::disableTrunkPorts(state);
        },
        "disable trunk ports to sync with LACP state");

    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(getTrunkCapacity(), 0); });
    bringUpPort(getAggPortMembers(kAggPort1(), *getAgentEnsemble())[0]);
    applyNewState(
        [](const std::shared_ptr<SwitchState>& state) {
          return utility::enableTrunkPorts(state);
        },
        "re-enable trunk member port");

    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(getTrunkCapacity(), member0Speed + member1Speed);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
