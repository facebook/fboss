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
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"
#include "fboss/lib/CommonUtils.h"

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
  }
};

TEST_F(AgentEcmpTest, CreateMaxEcmpGroups) {
  const auto kMaxEcmpGroup =
      utility::getMaxEcmpGroups(utility::getFirstAsic(this->getSw()));
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
    std::vector<flat_set<PortDescriptor>> newCombinations;
    for (auto& combination : allCombinations) {
      newCombinations.push_back(
          flat_set<PortDescriptor>{combination.begin(), combination.end()});
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
    ecmpHelper.programRoutes(&wrapper, newCombinations, prefixes);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

} // namespace facebook::fboss
