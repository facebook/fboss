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
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

class AgentEcmpTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::ECMP_LOAD_BALANCER};
  }

 protected:
  void SetUp() override {
    AgentHwTest::SetUp();
  }
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_ecmp_resource_percentage = 100;
    FLAGS_ecmp_width = 512;
  }
};

TEST_F(AgentEcmpTest, CreateMaxEcmpGroups) {
  const auto kMaxEcmpGroup =
      utility::getMaxEcmpGroups(getAgentEnsemble()->getL3Asics());
  auto setup = [&]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    std::vector<PortID> portIds = masterLogicalInterfacePortIds();
    std::vector<PortDescriptor> portDescriptorIds;
    std::vector<RoutePrefixV6> prefixes;
    for (auto& portId : portIds) {
      portDescriptorIds.push_back(PortDescriptor(portId));
    }
    std::vector<std::vector<PortDescriptor>> allCombinations =
        utility::generateEcmpGroupScale(portDescriptorIds, kMaxEcmpGroup);
    std::vector<flat_set<PortDescriptor>> nhopSets;
    for (const auto& combination : allCombinations) {
      nhopSets.emplace_back(combination.begin(), combination.end());
    }
    applyNewState([&portDescriptorIds,
                   &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(
          in,
          flat_set<PortDescriptor>(
              std::make_move_iterator(portDescriptorIds.begin()),
              std::make_move_iterator(portDescriptorIds.end())));
    });

    std::generate_n(
        std::back_inserter(prefixes), kMaxEcmpGroup, [i = 0]() mutable {
          return RoutePrefixV6{
              folly::IPAddressV6(folly::to<std::string>(2401, "::", i++)), 128};
        });
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, nhopSets, prefixes);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentEcmpTest, CreateMaxEcmpMembers) {
  const auto kMaxEcmpMembers =
      utility::getMaxEcmpMembers(getAgentEnsemble()->getL3Asics());
  auto setup = [&]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    std::vector<PortID> portIds = masterLogicalInterfacePortIds();
    std::vector<PortDescriptor> portDescriptorIds;
    std::vector<RoutePrefixV6> prefixes;
    for (const auto& portId : portIds) {
      portDescriptorIds.push_back(PortDescriptor(portId));
    }
    std::vector<std::vector<PortDescriptor>> allCombinations =
        utility::generateEcmpMemberScale(portDescriptorIds, kMaxEcmpMembers);
    std::vector<flat_set<PortDescriptor>> nhopSets;
    for (const auto& combination : allCombinations) {
      nhopSets.emplace_back(combination.begin(), combination.end());
    }
    applyNewState([&portDescriptorIds,
                   &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(
          in,
          flat_set<PortDescriptor>(
              std::make_move_iterator(portDescriptorIds.begin()),
              std::make_move_iterator(portDescriptorIds.end())));
    });
    std::generate_n(
        std::back_inserter(prefixes),
        allCombinations.size(),
        [i = 0]() mutable {
          return RoutePrefixV6{
              folly::IPAddressV6(folly::to<std::string>(2401, "::", i++)), 128};
        });
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, nhopSets, prefixes);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentEcmpTest, CreateMaxUcmpMembers) {
  const auto kMaxUcmpMembers =
      utility::getMaxUcmpMembers(getAgentEnsemble()->getL3Asics());

  auto setup = [&]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    std::vector<PortID> portIds = masterLogicalInterfacePortIds();
    std::vector<PortDescriptor> portDescriptorIds;
    std::vector<RoutePrefixV6> prefixes;
    std::vector<std::vector<PortDescriptor>> allCombinations;
    std::vector<std::vector<NextHopWeight>> swWeights;
    for (const auto& portId : portIds) {
      portDescriptorIds.emplace_back(portId);
    }
    applyNewState([&portDescriptorIds,
                   &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(
          in,
          flat_set<PortDescriptor>(
              std::make_move_iterator(portDescriptorIds.begin()),
              std::make_move_iterator(portDescriptorIds.end())));
    });
    if (isSupportedOnAllAsics(HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER)) {
      allCombinations =
          utility::generateEcmpMemberScale(portDescriptorIds, kMaxUcmpMembers);
      utility::assignUcmpWeights(allCombinations, swWeights);
    } else {
      allCombinations =
          utility::generateEcmpMemberScale(portDescriptorIds, kMaxUcmpMembers);
      allCombinations = utility::getUcmpMembersAndWeight(
          allCombinations, swWeights, kMaxUcmpMembers);
    }

    std::vector<flat_set<PortDescriptor>> nhopSets;
    for (const auto& combination : allCombinations) {
      nhopSets.emplace_back(combination.begin(), combination.end());
    }
    std::generate_n(
        std::back_inserter(prefixes), nhopSets.size(), [i = 0]() mutable {
          return RoutePrefixV6{
              folly::IPAddressV6(folly::to<std::string>(2401, "::", i++)), 128};
        });
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, nhopSets, prefixes, swWeights);
  };
  verifyAcrossWarmBoots(setup, [] {});
}
} // namespace facebook::fboss
