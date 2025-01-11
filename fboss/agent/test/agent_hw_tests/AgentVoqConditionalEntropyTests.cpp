// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

namespace facebook::fboss {

class AgentVoqSwitchConditionalEntropyTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchTest::initialConfig(ensemble);
    // Enable Conditional Entropy on Interface Ports
    for (auto& port : *cfg.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT) {
        port.conditionalEntropyRehash() = true;
      }
    }
    cfg.switchSettings()->conditionalEntropyRehashPeriodUS() = 100;
    return cfg;
  }
};

TEST_F(AgentVoqSwitchConditionalEntropyTest, init) {
  auto setup = []() {};

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (const auto& portMap : std::as_const(*state->getPorts())) {
      for (const auto& port : std::as_const(*portMap.second)) {
        if (port.second->getPortType() == cfg::PortType::INTERFACE_PORT) {
          EXPECT_TRUE(port.second->getConditionalEntropyRehash());
        }
      }
    }
    // TODO: Program ECMP route, insert traffic and verify change in next hop.
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchConditionalEntropyTest, verifyLoadBalancing) {
  const auto kEcmpWidth = 4;
  const auto kMaxDeviation = 25;

  auto getLocalSysPortDesc = [this](auto ecmpHelper) {
    std::vector<PortDescriptor> sysPortDescs;
    auto localSysPortDescs = resolveLocalNhops(ecmpHelper);
    sysPortDescs.insert(
        sysPortDescs.end(),
        localSysPortDescs.begin(),
        localSysPortDescs.begin() + kEcmpWidth);
    return sysPortDescs;
  };
  auto setup = [this, kEcmpWidth, &getLocalSysPortDesc]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());

    std::vector<PortDescriptor> sysPortDescs = getLocalSysPortDesc(ecmpHelper);

    auto prefix = RoutePrefixV6{folly::IPAddressV6("0::0"), 0};
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        flat_set<PortDescriptor>(
            std::make_move_iterator(sysPortDescs.begin()),
            std::make_move_iterator(sysPortDescs.end())),
        {prefix});
  };

  auto verify = [this, &getLocalSysPortDesc]() {
    // Send traffic through the 5th interface port and verify load balancing
    const auto kIngressPort = 5;
    CHECK(masterLogicalInterfacePortIds().size() > kIngressPort + 1);

    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    std::vector<PortDescriptor> sysPortDescs = getLocalSysPortDesc(ecmpHelper);

    std::function<std::map<SystemPortID, HwSysPortStats>(
        const std::vector<SystemPortID>&)>
        getSysPortStatsFn = [&](const std::vector<SystemPortID>& portIds) {
          getSw()->updateStats();
          return getLatestSysPortStats(portIds);
        };

    utility::pumpTrafficAndVerifyLoadBalanced(
        [&]() {
          utility::pumpRoCETraffic(
              true /* isV6 */,
              utility::getAllocatePktFn(getAgentEnsemble()),
              utility::getSendPktFunc(getAgentEnsemble()),
              utility::getFirstInterfaceMac(getProgrammedState()),
              std::nullopt /* vlan */,
              masterLogicalInterfacePortIds()[kIngressPort],
              utility::kUdfL4DstPort,
              255 /* hopLimit */,
              std::nullopt /* srcMacAddr */,
              1000000, /* packetCount */
              utility::kUdfRoceOpcodeAck,
              0x40, /* reserved */
              std::nullopt, /* nextHdr */
              true /* sameDstQueue */);
        },
        [&]() {
          auto ports = std::make_unique<std::vector<int32_t>>();
          for (auto sysPortDecs : sysPortDescs) {
            ports->push_back(static_cast<int32_t>(sysPortDecs.sysPortID()));
          }
          getSw()->clearPortStats(ports);
        },
        [&]() {
          WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(utility::isLoadBalanced(
              sysPortDescs, {}, getSysPortStatsFn, kMaxDeviation, false)));
          return true;
        });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
