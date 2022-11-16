/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fboss/agent/RouteUpdateWrapper.h>
#include <fboss/agent/hw/test/LoadBalancerUtils.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "folly/IPAddressV4.h"

#include "fboss/agent/state/SwitchState.h"

#include <folly/Optional.h>

#include <boost/container/flat_set.hpp>

#include <vector>

using boost::container::flat_set;
using std::vector;

using facebook::fboss::utility::getEcmpFullTrunkHalfHashConfig;
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

enum TrafficType {
  IPv4,
  IPv6,
  IPv4MPLS,
  IPv6MPLS,
  v4MPLS4Swap,
  v4MPLS4Php,
  v6MPLS4Swap,
  v6MPLS4Php
};

static constexpr uint32_t kV4TopSwap = 10010;
static constexpr uint32_t kV6TopSwap = 10011;
static constexpr uint32_t kV4TopPhp = 20010;
static constexpr uint32_t kV6TopPhp = 20011;

} // namespace

namespace facebook::fboss {

class HwTrunkLoadBalancerTest : public HwLinkStateDependentTest {
 private:
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
    auto state = applyNewConfig(config);
    applyNewState(utility::enableTrunkPorts(state));
  }

  std::unique_ptr<HwSwitchEnsembleRouteUpdateWrapper> getRouteUpdater() {
    return std::make_unique<HwSwitchEnsembleRouteUpdateWrapper>(
        getHwSwitchEnsemble()->getRouteUpdater());
  }

  template <typename ECMP_HELPER>
  void programRoutes(const ECMP_HELPER& ecmpHelper, const AggPortInfo aggInfo) {
    applyNewState(ecmpHelper.resolveNextHops(
        getProgrammedState(), getAggregatePorts(aggInfo)));
    ecmpHelper.programRoutes(getRouteUpdater(), getAggregatePorts(aggInfo));
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

    applyNewState(ecmpHelper.resolveNextHops(
        getProgrammedState(), getAggregatePorts(aggInfo)));
    ecmpHelper.programIp2MplsRoutes(
        getRouteUpdater(), getAggregatePorts(aggInfo), stacks);
  }

  template <typename ECMP_HELPER>
  void programMplsRoutes(
      const ECMP_HELPER& ecmpHelper,
      const AggPortInfo aggInfo) {
    applyNewState(ecmpHelper.resolveNextHops(
        getProgrammedState(), getAggregatePorts(aggInfo)));
    auto ports = getAggregatePorts(aggInfo);
    ecmpHelper.setupECMPForwarding(
        getProgrammedState(), getRouteUpdater(), ports);
  }

 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    return config;
  }

  void
  pumpIPTraffic(bool isV6, bool loopThroughFrontPanel, AggPortInfo aggInfo) {
    std::optional<PortID> frontPanelPortToLoopTraffic;
    if (loopThroughFrontPanel) {
      // Next port to loop back traffic through
      frontPanelPortToLoopTraffic =
          PortID(masterLogicalPortIds()[aggInfo.numPhysicalPorts()]);
    }
    auto firstVlanID = getProgrammedState()->getVlans()->getFirstVlanID();
    auto mac = utility::getInterfaceMac(getProgrammedState(), firstVlanID);
    utility::pumpTraffic(
        isV6, getHwSwitch(), mac, firstVlanID, frontPanelPortToLoopTraffic);
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
    auto firstVlanID = getProgrammedState()->getVlans()->getFirstVlanID();
    auto mac = utility::getInterfaceMac(getProgrammedState(), firstVlanID);
    utility::pumpMplsTraffic(
        isV6,
        getHwSwitch(),
        label,
        mac,
        firstVlanID,
        frontPanelPortToLoopTraffic);
  }

  cfg::SwitchConfig configureAggregatePorts(AggPortInfo aggInfo) {
    auto config = initialConfig();
    addAggregatePorts(&config, aggInfo);
    return config;
  }

  void setupIPECMP(AggPortInfo aggInfo) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{getProgrammedState()};
    utility::EcmpSetupTargetedPorts4 ecmpHelper4{getProgrammedState()};
    programRoutes(ecmpHelper6, aggInfo);
    programRoutes(ecmpHelper4, aggInfo);
  }

  void setupIP2MPLSECMP(AggPortInfo aggInfo) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{getProgrammedState()};
    utility::EcmpSetupTargetedPorts4 ecmpHelper4{getProgrammedState()};
    programIp2MplsRoutes(ecmpHelper6, aggInfo);
    programIp2MplsRoutes(ecmpHelper4, aggInfo);
  }

  void setupMPLSECMP(AggPortInfo aggInfo) {
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV4> v4SwapHelper{
        getProgrammedState(),
        kV4TopSwap,
        LabelForwardingAction::LabelForwardingType::SWAP};
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6> v6SwapHelper{
        getProgrammedState(),
        kV6TopSwap,
        LabelForwardingAction::LabelForwardingType::SWAP};
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV4> v4PhpHelper{
        getProgrammedState(),
        kV4TopPhp,
        LabelForwardingAction::LabelForwardingType::PHP};
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV4> v6PhpHelper{
        getProgrammedState(),
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
    getHwSwitch()->clearPortStats(ports);
  }

  void pumpIPTrafficAndVerifyLoadBalanced(
      bool isV6,
      bool loopThroughFrontPanel,
      AggPortInfo aggInfo,
      int deviation) {
    auto isLoadBalanced = utility::pumpTrafficAndVerifyLoadBalanced(
        [=]() { pumpIPTraffic(isV6, loopThroughFrontPanel, aggInfo); },
        [=]() { clearPortStats(aggInfo); },
        [=]() {
          return utility::isLoadBalanced(
              getHwSwitchEnsemble(), getPhysicalPorts(aggInfo), deviation);
        });
    EXPECT_TRUE(isLoadBalanced);
  }

  void pumpMPLSTrafficAndVerifyLoadBalanced(
      bool isV6,
      uint32_t labelV4,
      uint32_t labelV6,
      bool loopThroughFrontPanel,
      AggPortInfo aggInfo,
      int deviation) {
    auto isLoadBalanced = utility::pumpTrafficAndVerifyLoadBalanced(
        [=]() {
          if (isV6) {
            pumpMPLSTraffic(isV6, labelV6, loopThroughFrontPanel, aggInfo);
          } else {
            pumpMPLSTraffic(isV6, labelV4, loopThroughFrontPanel, aggInfo);
          }
        },
        [=]() { clearPortStats(aggInfo); },
        [=]() {
          return utility::isLoadBalanced(
              getHwSwitchEnsemble(), getPhysicalPorts(aggInfo), deviation);
        });
    EXPECT_TRUE(isLoadBalanced);
  }

  void runIPLoadBalanceTest(
      bool isV6,
      const std::vector<cfg::LoadBalancer>& loadBalancers,
      AggPortInfo aggInfo,
      bool loopThroughFrontPanel,
      int deviation) {
    auto setup = [=]() {
      auto config = configureAggregatePorts(aggInfo);
      applyConfigAndEnableTrunks(config);
      setupIPECMP(aggInfo);
      applyNewState(utility::addLoadBalancers(
          getPlatform(), getProgrammedState(), loadBalancers));
    };
    auto verify = [=]() {
      pumpIPTrafficAndVerifyLoadBalanced(
          isV6, loopThroughFrontPanel, aggInfo, deviation);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runIP2MplsLoadBalanceTest(
      bool isV6,
      const std::vector<cfg::LoadBalancer>& loadBalancers,
      AggPortInfo aggInfo,
      bool loopThroughFrontPanel,
      int deviation) {
    if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::MPLS)) {
#if defined GTEST_SKIP
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=]() {
      auto config = configureAggregatePorts(aggInfo);
      applyConfigAndEnableTrunks(config);
      setupIP2MPLSECMP(aggInfo);
      applyNewState(utility::addLoadBalancers(
          getPlatform(), getProgrammedState(), loadBalancers));
    };
    auto verify = [=]() {
      pumpIPTrafficAndVerifyLoadBalanced(
          isV6, loopThroughFrontPanel, aggInfo, deviation);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runMpls2MplsLoadBalanceTest(
      bool isV6,
      const std::vector<cfg::LoadBalancer>& loadBalancers,
      LabelForwardingAction::LabelForwardingType type,
      AggPortInfo aggInfo,
      bool loopThroughFrontPanel,
      int deviation) {
    if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::MPLS) ||
        !getPlatform()->getAsic()->isSupported(HwAsic::Feature::MPLS_ECMP)) {
#if defined GTEST_SKIP
      GTEST_SKIP();
#endif
      return;
    }
    uint32_t labelV4 =
        (type == LabelForwardingAction::LabelForwardingType::SWAP ? kV4TopSwap
                                                                  : kV4TopPhp);
    uint32_t labelV6 =
        (type == LabelForwardingAction::LabelForwardingType::SWAP ? kV6TopSwap
                                                                  : kV6TopPhp);
    auto setup = [=]() {
      auto config = configureAggregatePorts(aggInfo);
      applyConfigAndEnableTrunks(config);
      setupMPLSECMP(aggInfo);
      applyNewState(utility::addLoadBalancers(
          getPlatform(), getProgrammedState(), loadBalancers));
    };
    auto verify = [=]() {
      pumpMPLSTrafficAndVerifyLoadBalanced(
          isV6, labelV4, labelV6, loopThroughFrontPanel, aggInfo, 25);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runMpls2MplsLoadBalanceTest(
      bool isV6,
      const std::vector<cfg::LoadBalancer>& loadBalancers,
      LabelForwardingAction::LabelForwardingType type,
      AggPortInfo aggInfo,
      bool loopThroughFrontPanel) {
    uint32_t labelV4 =
        (type == LabelForwardingAction::LabelForwardingType::SWAP ? kV4TopSwap
                                                                  : kV4TopPhp);
    uint32_t labelV6 =
        (type == LabelForwardingAction::LabelForwardingType::SWAP ? kV6TopSwap
                                                                  : kV6TopPhp);
    auto setup = [=]() {
      auto config = configureAggregatePorts(aggInfo);
      applyNewConfig(config);
      setupMPLSECMP(aggInfo);
      applyNewState(utility::addLoadBalancers(
          getPlatform(), getProgrammedState(), loadBalancers));
    };
    auto verify = [=]() {
      pumpMPLSTrafficAndVerifyLoadBalanced(
          isV6, labelV4, labelV6, loopThroughFrontPanel, aggInfo, 25);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runLoadBalanceTest(
      TrafficType traffic,
      const std::vector<cfg::LoadBalancer>& loadBalancers,
      AggPortInfo aggInfo,
      bool loopThroughFrontPanel = false,
      int deviation = 25) {
    switch (traffic) {
      case TrafficType::IPv4:
        return runIPLoadBalanceTest(
            false, loadBalancers, aggInfo, loopThroughFrontPanel, deviation);
      case TrafficType::IPv6:
        return runIPLoadBalanceTest(
            true, loadBalancers, aggInfo, loopThroughFrontPanel, deviation);
      case TrafficType::IPv4MPLS:
        return runIP2MplsLoadBalanceTest(
            false, loadBalancers, aggInfo, loopThroughFrontPanel, deviation);
      case TrafficType::IPv6MPLS:
        return runIP2MplsLoadBalanceTest(
            true, loadBalancers, aggInfo, loopThroughFrontPanel, deviation);
      case TrafficType::v4MPLS4Swap:
        return runMpls2MplsLoadBalanceTest(
            false,
            loadBalancers,
            LabelForwardingAction::LabelForwardingType::SWAP,
            aggInfo,
            loopThroughFrontPanel,
            deviation);
      case TrafficType::v6MPLS4Swap:
        return runMpls2MplsLoadBalanceTest(
            true,
            loadBalancers,
            LabelForwardingAction::LabelForwardingType::SWAP,
            aggInfo,
            loopThroughFrontPanel,
            deviation);
      case TrafficType::v4MPLS4Php:
        return runMpls2MplsLoadBalanceTest(
            false,
            loadBalancers,
            LabelForwardingAction::LabelForwardingType::PHP,
            aggInfo,
            loopThroughFrontPanel,
            deviation);
      case TrafficType::v6MPLS4Php:
        return runMpls2MplsLoadBalanceTest(
            true,
            loadBalancers,
            LabelForwardingAction::LabelForwardingType::PHP,
            aggInfo,
            loopThroughFrontPanel,
            deviation);
    }
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
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalfHash4X3WideTrunksV6CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs);
}
TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalfHash4X3WideTrunksV4CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV6FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV4FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV6MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6MPLS,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV4MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4MPLS,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV6MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Swap,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV4MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Swap,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV6MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Php,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X3WideTrunksV4MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Php,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalfHash4X2WideTrunksV6CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs);
}
TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalfHash4X2WideTrunksV4CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV6FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV4FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV6MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6MPLS,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV4MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4MPLS,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV6MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Swap,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV4MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Swap,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV6MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Php,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPFullTrunkHalf4X2WideTrunksV4MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Php,
      getEcmpFullTrunkHalfHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

// ECMP half hash, Trunk full hash tests
TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFullHash4X3WideTrunksV6CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs);
}
TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFullHash4X3WideTrunksV4CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV6FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV4FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV6MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6MPLS,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV4MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4MPLS,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV6MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Swap,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV4MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Swap,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV6MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Php,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X3WideTrunksV4MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Php,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X3WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFullHash4X2WideTrunksV6CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs);
}
TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFullHash4X2WideTrunksV4CpuTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV6FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV4FrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV6MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv6MPLS,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV4MplsFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::IPv4MPLS,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV6MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Swap,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV4MplsSwapFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Swap,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV6MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v6MPLS4Php,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(
    HwTrunkLoadBalancerTest,
    ECMPHalfTrunkFull4X2WideTrunksV4MplsPhpFrontPanelTraffic) {
  runLoadBalanceTest(
      TrafficType::v4MPLS4Php,
      getEcmpHalfTrunkFullHashConfig(getPlatform()),
      k4X2WideAggs,
      true /* loopThroughFrontPanelPort*/);
}
} // namespace facebook::fboss
