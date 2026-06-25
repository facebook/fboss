/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/AgentHwTest.h"

#include <fboss/agent/RouteUpdateWrapper.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/state/StateUtils.h"

#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "folly/IPAddressV4.h"

#include "fboss/agent/state/SwitchState.h"

#include <boost/container/flat_set.hpp>

#include <vector>

using boost::container::flat_set;
using std::vector;

using facebook::fboss::utility::getEcmpFullTrunkHalfHashConfig;
using facebook::fboss::utility::
    getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig;
using facebook::fboss::utility::getEcmpHalfTrunkFullHashConfig;

namespace {
struct AggPortInfo {
  uint numPhysicalPorts() const {
    return numAggPorts * aggPortWidth;
  }
  uint numAggPorts{4};
  uint aggPortWidth{3};
};
const AggPortInfo k4X3WideAggs{4, 3};
const AggPortInfo k4X2WideAggs{4, 2};

static constexpr uint32_t kV4TopSwap = 10010;
static constexpr uint32_t kV6TopSwap = 10011;
static constexpr uint32_t kV4TopPhp = 20010;
static constexpr uint32_t kV6TopPhp = 20011;

} // namespace

namespace facebook::fboss {
class AgentTrunkLoadBalancerTest : public AgentHwTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::LAG, ProductionFeature::LAG_LOAD_BALANCER};
  }

  void addAggregatePorts(cfg::SwitchConfig* config, AggPortInfo aggInfo) const {
    AggregatePortID curAggId{1};
    for (auto i = 0; i < aggInfo.numAggPorts; ++i) {
      std::vector<int32_t> members(aggInfo.aggPortWidth);
      for (auto j = 0; j < aggInfo.aggPortWidth; ++j) {
        members[j] = masterLogicalPortIds()[i * aggInfo.aggPortWidth + j];
      }
      utility::addAggPort(curAggId++, members, config);
    }
  }
  std::vector<PortDescriptor> getPhysicalPorts(AggPortInfo aggInfo) const {
    std::vector<PortDescriptor> physicalPorts;
    for (auto i = 0; i < aggInfo.numPhysicalPorts(); ++i) {
      physicalPorts.push_back(
          PortDescriptor(PortID(masterLogicalPortIds()[i])));
    }
    return physicalPorts;
  }
  flat_set<PortDescriptor> getAggregatePorts(AggPortInfo aggInfo) const {
    flat_set<PortDescriptor> aggregatePorts;
    for (auto i = 0; i < aggInfo.numAggPorts; ++i) {
      aggregatePorts.insert(PortDescriptor(AggregatePortID(i + 1)));
    }
    return aggregatePorts;
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& config) {
    applyNewConfig(config);
    applyNewState(
        [](const std::shared_ptr<SwitchState> state) {
          return utility::enableTrunkPorts(state);
        },
        "enable trunk ports");
  }

  template <typename ECMP_HELPER>
  void programRoutes(const ECMP_HELPER& ecmpHelper, const AggPortInfo aggInfo) {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      return ecmpHelper.resolveNextHops(out, getAggregatePorts(aggInfo));
    });
    ecmpHelper.programRoutes(
        std::make_unique<SwSwitchRouteUpdateWrapper>(
            getSw(), getSw()->getRib()),
        getAggregatePorts(aggInfo));
  }

  template <typename ECMP_HELPER>
  void programIp2MplsRoutes(
      const ECMP_HELPER& ecmpHelper,
      const AggPortInfo aggInfo) {
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks{};

    for (auto i = 0; i < aggInfo.numAggPorts; ++i) {
      stacks.emplace(
          PortDescriptor(AggregatePortID(i + 1)),
          LabelForwardingAction::LabelStack{10010 + i + 1});
    }

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      return ecmpHelper.resolveNextHops(out, getAggregatePorts(aggInfo));
    });
    ecmpHelper.programIp2MplsRoutes(
        std::make_unique<SwSwitchRouteUpdateWrapper>(
            getSw(), getSw()->getRib()),
        getAggregatePorts(aggInfo),
        stacks);
  }

  template <typename ECMP_HELPER>
  void programMplsRoutes(
      const ECMP_HELPER& ecmpHelper,
      const AggPortInfo aggInfo) {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      return ecmpHelper.resolveNextHops(out, getAggregatePorts(aggInfo));
    });
    auto ports = getAggregatePorts(aggInfo);
    ecmpHelper.setupECMPForwarding(
        getProgrammedState(),
        std::make_unique<SwSwitchRouteUpdateWrapper>(
            getSw(), getSw()->getRib()),
        ports);
  }

 protected:
  void
  pumpIPTraffic(bool isV6, bool loopThroughFrontPanel, AggPortInfo aggInfo) {
    std::optional<PortID> frontPanelPortToLoopTraffic;
    if (loopThroughFrontPanel) {
      // Next port to loop back traffic through
      frontPanelPortToLoopTraffic =
          PortID(masterLogicalPortIds()[aggInfo.numPhysicalPorts()]);
    }
    auto firstVlanID = getVlanIDForTx();
    auto mac = getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());

    utility::pumpTraffic(
        isV6,
        utility::getAllocatePktFn(getAgentEnsemble()),
        utility::getSendPktFunc(getAgentEnsemble()),
        mac,
        firstVlanID,
        frontPanelPortToLoopTraffic,
        255,
        20000);
  }

  void pumpMPLSTraffic(
      bool isV6,
      uint32_t label,
      bool loopThroughFrontPanel,
      AggPortInfo aggInfo) {
    std::optional<PortID> frontPanelPortToLoopTraffic;
    if (loopThroughFrontPanel) {
      // Next port to loop back traffic through
      frontPanelPortToLoopTraffic =
          PortID(masterLogicalPortIds()[aggInfo.numPhysicalPorts()]);
    }
    auto firstVlanID = getVlanIDForTx();
    auto mac = getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    utility::pumpMplsTraffic(
        isV6,
        utility::getAllocatePktFn(getAgentEnsemble()),
        utility::getSendPktFunc(getAgentEnsemble()),
        label,
        mac,
        firstVlanID.value(),
        frontPanelPortToLoopTraffic);
  }

  cfg::SwitchConfig configureAggregatePorts(AggPortInfo aggInfo) {
    auto config = initialConfig(*getAgentEnsemble());
    addAggregatePorts(&config, aggInfo);
    return config;
  }

  void setupIPECMP(AggPortInfo aggInfo) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{
        getProgrammedState(), getSw()->needL2EntryForNeighbor()};
    utility::EcmpSetupTargetedPorts4 ecmpHelper4{
        getProgrammedState(), getSw()->needL2EntryForNeighbor()};
    programRoutes(ecmpHelper6, aggInfo);
    programRoutes(ecmpHelper4, aggInfo);
  }

  void setupIP2MPLSECMP(AggPortInfo aggInfo) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{
        getProgrammedState(), getSw()->needL2EntryForNeighbor()};
    utility::EcmpSetupTargetedPorts4 ecmpHelper4{
        getProgrammedState(), getSw()->needL2EntryForNeighbor()};
    programIp2MplsRoutes(ecmpHelper6, aggInfo);
    programIp2MplsRoutes(ecmpHelper4, aggInfo);
  }

  void setupMPLSECMP(AggPortInfo aggInfo) {
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV4> v4SwapHelper{
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        kV4TopSwap,
        LabelForwardingAction::LabelForwardingType::SWAP};
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6> v6SwapHelper{
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        kV6TopSwap,
        LabelForwardingAction::LabelForwardingType::SWAP};
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV4> v4PhpHelper{
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        kV4TopPhp,
        LabelForwardingAction::LabelForwardingType::PHP};
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV4> v6PhpHelper{
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        kV6TopPhp,
        LabelForwardingAction::LabelForwardingType::PHP};
    programMplsRoutes(v4SwapHelper, aggInfo);
    programMplsRoutes(v6SwapHelper, aggInfo);
    programMplsRoutes(v4PhpHelper, aggInfo);
    programMplsRoutes(v6PhpHelper, aggInfo);
  }

  void clearPortStats(AggPortInfo aggInfo) {
    auto portDescs = getPhysicalPorts(aggInfo);
    auto ports = std::make_unique<std::vector<int32_t>>();
    for (auto portDesc : portDescs) {
      ports->push_back(static_cast<int32_t>(portDesc.phyPortID()));
    }
    getSw()->clearPortStats(ports);
  }

  void pumpIPTrafficAndVerifyLoadBalanced(
      bool isV6,
      bool loopThroughFrontPanel,
      AggPortInfo aggInfo,
      int deviation) {
    utility::pumpTrafficAndVerifyLoadBalanced(
        [=, this]() { pumpIPTraffic(isV6, loopThroughFrontPanel, aggInfo); },
        [=, this]() { clearPortStats(aggInfo); },
        [=, this]() {
          return utility::isLoadBalanced<PortID, HwPortStats>(
              getPhysicalPorts(aggInfo),
              std::vector<NextHopWeight>(),
              [=, this](const std::vector<PortID>& portIds) {
                return this->getLatestPortStats(portIds);
              },
              deviation);
        });
  }

  void pumpIPv6FlowLabelTraffic(
      bool loopThroughFrontPanel,
      AggPortInfo aggInfo) {
    std::optional<PortID> frontPanelPortToLoopTraffic;
    if (loopThroughFrontPanel) {
      // Next port to loop back traffic through
      frontPanelPortToLoopTraffic =
          PortID(masterLogicalPortIds()[aggInfo.numPhysicalPorts()]);
    }
    auto firstVlanID = getVlanIDForTx();
    auto mac = getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    // Vary only the IPv6 flow label (fixed src/dst addr + L4 ports) so the
    // test exercises flow-label based hashing specifically.
    utility::pumpTrafficWithFlowLabel(
        utility::getAllocatePktFn(getAgentEnsemble()),
        utility::getSendPktFunc(getAgentEnsemble()),
        mac,
        firstVlanID,
        frontPanelPortToLoopTraffic,
        255,
        20000);
  }

  void pumpIPv6FlowLabelTrafficAndVerifyLoadBalanced(
      bool loopThroughFrontPanel,
      AggPortInfo aggInfo,
      int deviation) {
    utility::pumpTrafficAndVerifyLoadBalanced(
        [=, this]() {
          pumpIPv6FlowLabelTraffic(loopThroughFrontPanel, aggInfo);
        },
        [=, this]() { clearPortStats(aggInfo); },
        [=, this]() {
          return utility::isLoadBalanced<PortID, HwPortStats>(
              getPhysicalPorts(aggInfo),
              std::vector<NextHopWeight>(),
              [=, this](const std::vector<PortID>& portIds) {
                return this->getLatestPortStats(portIds);
              },
              deviation);
        });
  }

  void pumpMPLSTrafficAndVerifyLoadBalanced(
      bool isV6,
      uint32_t labelV4,
      uint32_t labelV6,
      bool loopThroughFrontPanel,
      AggPortInfo aggInfo,
      int deviation) {
    utility::pumpTrafficAndVerifyLoadBalanced(
        [=, this]() {
          if (isV6) {
            pumpMPLSTraffic(isV6, labelV6, loopThroughFrontPanel, aggInfo);
          } else {
            pumpMPLSTraffic(isV6, labelV4, loopThroughFrontPanel, aggInfo);
          }
        },
        [=, this]() { clearPortStats(aggInfo); },
        [=, this]() {
          return utility::isLoadBalanced<PortID, HwPortStats>(
              getPhysicalPorts(aggInfo),
              std::vector<NextHopWeight>(),
              [=, this](const std::vector<PortID>& portIds) {
                return this->getLatestPortStats(portIds);
              },
              deviation);
        });
  }

  // Test categories. A category is family-agnostic: it describes the route/ECMP
  // programming and the kind of traffic to pump, but not the IP family (v4 vs
  // v6). Each merged TEST_F programs a category once (setup) and verifies both
  // families (verify), so that verifyAcrossWarmBoots is invoked exactly once
  // per test.
  enum class LoadBalanceTestType {
    IP,
    IP2Mpls,
    Mpls2MplsSwap,
    Mpls2MplsPhp,
  };

  // Family-agnostic setup: programs config, trunks and ECMP/routes once.
  void setupLoadBalanceTest(
      LoadBalanceTestType testType,
      const std::vector<cfg::LoadBalancer>& loadBalancers,
      AggPortInfo aggInfo) {
    auto config = configureAggregatePorts(aggInfo);
    config.loadBalancers() = loadBalancers;
    applyConfigAndEnableTrunks(config);
    switch (testType) {
      case LoadBalanceTestType::IP:
        setupIPECMP(aggInfo);
        break;
      case LoadBalanceTestType::IP2Mpls:
        setupIP2MPLSECMP(aggInfo);
        break;
      case LoadBalanceTestType::Mpls2MplsSwap:
      case LoadBalanceTestType::Mpls2MplsPhp:
        setupMPLSECMP(aggInfo);
        break;
    }
  }

  // Per-family verify: pumps traffic for the given family and asserts load
  // balancing.
  void verifyLoadBalanceTest(
      LoadBalanceTestType testType,
      bool isV6,
      AggPortInfo aggInfo,
      bool loopThroughFrontPanel,
      int deviation) {
    switch (testType) {
      case LoadBalanceTestType::IP:
      case LoadBalanceTestType::IP2Mpls:
        pumpIPTrafficAndVerifyLoadBalanced(
            isV6, loopThroughFrontPanel, aggInfo, deviation);
        break;
      case LoadBalanceTestType::Mpls2MplsSwap:
        pumpMPLSTrafficAndVerifyLoadBalanced(
            isV6, kV4TopSwap, kV6TopSwap, loopThroughFrontPanel, aggInfo, 25);
        break;
      case LoadBalanceTestType::Mpls2MplsPhp:
        pumpMPLSTrafficAndVerifyLoadBalanced(
            isV6, kV4TopPhp, kV6TopPhp, loopThroughFrontPanel, aggInfo, 25);
        break;
    }
  }

  // @lint-ignore-every CLANGTIDY
  // Runs a merged v4 + v6 load balance test with a single verifyAcrossWarmBoots
  // invocation: setup programs the (family-agnostic) state once, verify pumps
  // and checks v4 then v6.
  void runLoadBalanceTest(
      LoadBalanceTestType testType,
      const std::vector<cfg::LoadBalancer>& loadBalancers,
      AggPortInfo aggInfo,
      bool loopThroughFrontPanel = false,
      int deviation = 25) {
    auto setup = [=, this]() {
      setupLoadBalanceTest(testType, loadBalancers, aggInfo);
    };
    auto verify = [=, this]() {
      {
        SCOPED_TRACE("v4");
        verifyLoadBalanceTest(
            testType,
            false /* isV6 */,
            aggInfo,
            loopThroughFrontPanel,
            deviation);
      }
      {
        SCOPED_TRACE("v6");
        verifyLoadBalanceTest(
            testType,
            true /* isV6 */,
            aggInfo,
            loopThroughFrontPanel,
            deviation);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

// MPLS Trunk + ECMP load balancing
class AgentMplsTrunkLoadBalancerTest : public AgentTrunkLoadBalancerTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::MPLS,
        ProductionFeature::LAG,
        ProductionFeature::LAG_LOAD_BALANCER};
  }
};

// SRv6 Trunk + ECMP load balancing
class AgentSrv6TrunkLoadBalancerTest : public AgentTrunkLoadBalancerTest {
 protected:
  static constexpr AggPortInfo kSrv6AggInfo{1, 2};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::SRV6_ENCAP,
        ProductionFeature::LAG,
        ProductionFeature::LAG_LOAD_BALANCER};
  }

  void setupSrv6TrunkECMP() {
    auto config = configureAggregatePorts(kSrv6AggInfo);
    // Add SRv6 tunnels
    std::vector<cfg::Srv6Tunnel> tunnelList;
    for (int i = 0; i < kSrv6AggInfo.numAggPorts; ++i) {
      tunnelList.push_back(
          utility::makeSrv6TunnelConfig(
              folly::sformat("srv6Tunnel{}", i),
              InterfaceID(config.interfaces()[i * kSrv6AggInfo.aggPortWidth]
                              .intfID()
                              .value())));
    }
    config.srv6Tunnels() = tunnelList;
    config.loadBalancers() =
        getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig(
            getAgentEnsemble()->getL3Asics());
    applyConfigAndEnableTrunks(config);

    // Resolve neighbors on aggregate ports and program SRv6 routes
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), getSw()->needL2EntryForNeighbor()};
    auto aggPorts = getAggregatePorts(kSrv6AggInfo);

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, aggPorts);
    });

    RouteNextHopSet nhops;
    for (int i = 0; i < kSrv6AggInfo.numAggPorts; ++i) {
      auto aggPortDesc = PortDescriptor(AggregatePortID(i + 1));
      auto nhop = ecmpHelper.nhop(aggPortDesc);
      std::vector<folly::IPAddressV6> sidList{
          folly::IPAddressV6(folly::sformat("3001:db8:{}::", i + 1))};
      nhops.insert(ResolvedNextHop(
          nhop.ip,
          nhop.intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          sidList,
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0")));
    }
    auto routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2001::"),
        32,
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
    routeUpdater.program();
  }
};

// IPv6 flow label Trunk + ECMP load balancing
class AgentV6FlowLabelTrunkLoadBalancerTest
    : public AgentTrunkLoadBalancerTest {
 protected:
  void setupV6FlowLabelTrunkECMP(AggPortInfo aggInfo) {
    auto config = configureAggregatePorts(aggInfo);
    // Hash on the IPv6 flow label for both ECMP and trunk (AGGREGATE_PORT).
    config.loadBalancers() =
        getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig(
            getAgentEnsemble()->getL3Asics());
    applyConfigAndEnableTrunks(config);
    setupIPECMP(aggInfo);
  }
};

/*
 * We test for 2 combinations -
 * i) 4X3Wide Four 3 wide trunks in ECMP group
 * ii) 4X2Wide Four 2 wide trunks in ECMP group.
 * The reason for this is that is to isolate polarization bugs. We have had
 * cases - T39279972, where we ended up mis programming trunk hashing in a way
 * that both ECMP and trunks were using the same 16 bits (out of 83) of hash
 * function output. This then would result in polarization in **some** MXN
 * combinations of trunk members of ECMP And width of trunks. For example
 * consider that the 16 bits selected result sequence of numbers
 * {X1, X2, ... XN}  for a set of flows. Further consider that these are the
 * set of flows that are sent of 0th member of the ECMP group. Implying that
 * each of {X1%M, X2%M ...} would be 0. Since we picked the same 16 bits of
 * hash for trunks. We would now be doing {X1%N, X2%N ...} for selecting the
 * trunk member link. So to expose polarization bugs, we need to pick a
 * combination of MXN where {X1%N, X2%N ...} would result in subset of links to
 * be used. 2 way ECMP over 2 wide trunks presents such a combination (4X4, 4X2
 * etc would be others). Consider the case of 4X2, on the 0th ECMP member link.
 * Assume that the bits selected here lead to numbers
 * {0, 4, 8, 12, 16 ..}. The actual numbers are not important, the only thing
 * important is that they all yield 0 when doing %4 on them. Now when we apply
 * %2 for selecting the trunk member, we would always end up selecting the
 * first link.
 */
// ECMP full hash, Trunk half hash tests
TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPFullTrunkHalfHash4X3WideTrunksCpuTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs);
}

TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPFullTrunkHalfHash4X2WideTrunksCpuTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs);
}

TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

// ECMP half hash, Trunk full hash IP tests
TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPHalfTrunkFullHash4X3WideTrunksCpuTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs);
}

TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPHalfTrunkFullHash4X2WideTrunksCpuTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs);
}

TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksMplsFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP2Mpls,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksMplsFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP2Mpls,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksMplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsSwap,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksMplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsSwap,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksMplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsPhp,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksMplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsPhp,
      getEcmpFullTrunkHalfHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

// ECMP half hash, Trunk full hash tests
TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksMplsFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP2Mpls,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksMplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsSwap,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksMplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsPhp,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksMplsFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::IP2Mpls,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksMplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsSwap,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    AgentMplsTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksMplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      LoadBalanceTestType::Mpls2MplsPhp,
      getEcmpHalfTrunkFullHashConfig(getAgentEnsemble()->getL3Asics()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(AgentSrv6TrunkLoadBalancerTest, Srv6TrunkEcmpLoadBalance) {
  auto setup = [this]() { setupSrv6TrunkECMP(); };
  auto verify = [this]() {
    pumpIPTrafficAndVerifyLoadBalanced(
        true /* isV6 */,
        false /* loopThroughFrontPanel */,
        kSrv6AggInfo,
        25 /* deviation */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(
    AgentV6FlowLabelTrunkLoadBalancerTest,
    V6FlowLabelEcmpTrunk4X3WideTrunksCpuTraffic) {
  auto setup = [this]() { setupV6FlowLabelTrunkECMP(k4X3WideAggs); };
  auto verify = [this]() {
    pumpIPv6FlowLabelTrafficAndVerifyLoadBalanced(
        false /* loopThroughFrontPanel */, k4X3WideAggs, 25 /* deviation */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(
    AgentV6FlowLabelTrunkLoadBalancerTest,
    V6FlowLabelEcmpTrunk4X3WideTrunksFrontPanelTraffic) {
  auto setup = [this]() { setupV6FlowLabelTrunkECMP(k4X3WideAggs); };
  auto verify = [this]() {
    pumpIPv6FlowLabelTrafficAndVerifyLoadBalanced(
        true /* loopThroughFrontPanel */, k4X3WideAggs, 25 /* deviation */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
