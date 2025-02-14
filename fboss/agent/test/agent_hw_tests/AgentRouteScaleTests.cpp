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
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

namespace facebook::fboss {

class AgentRouteScaleTest : public AgentHwTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_FORWARDING};
  }

  void programRoutes(
      RouteUpdateWrapper& updater,
      RouterID rid,
      ClientID client,
      const utility::RouteDistributionGenerator::ThriftRouteChunks&
          routeChunks) {
    for (const auto& routeChunk : routeChunks) {
      std::for_each(
          routeChunk.begin(),
          routeChunk.end(),
          [client, rid, &updater](const auto& route) {
            updater.addRoute(rid, client, route);
          });
      updater.program();
    }
  }

  template <typename RouteScaleGeneratorT>
  void runTest() {
    auto setup = [this]() {
      auto routeGen = RouteScaleGeneratorT(getProgrammedState());

      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return routeGen.resolveNextHops(in);
      });
      auto routeChunks = routeGen.getThriftRoutes();
      auto updater = getSw()->getRouteUpdater();
      programRoutes(updater, RouterID(0), ClientID::BGPD, routeChunks);
    };
    auto verify = [] {};
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void setCmdLineFlagOverrides() const override {
    FLAGS_enable_route_resource_protection = false;
    // Turn on Leaba SDK shadow cache to avoid test case timeout
    FLAGS_counter_refresh_interval = 1;
  }
};

class AgentRswRouteScaleTest : public AgentRouteScaleTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto features = AgentRouteScaleTest::getProductionFeaturesVerified();
    features.push_back(production_features::ProductionFeature::RSW_ROUTE_SCALE);
    return features;
  }
};

TEST_F(AgentRswRouteScaleTest, rswRouteScale) {
  runTest<utility::RSWRouteScaleGenerator>();
}

class AgentFswRouteScaleTest : public AgentRouteScaleTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto features = AgentRouteScaleTest::getProductionFeaturesVerified();
    features.push_back(production_features::ProductionFeature::FSW_ROUTE_SCALE);
    return features;
  }
};

TEST_F(AgentFswRouteScaleTest, fswRouteScale) {
  runTest<utility::FSWRouteScaleGenerator>();
}

class AgentThAlpmRouteScaleTest : public AgentRouteScaleTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto features = AgentRouteScaleTest::getProductionFeaturesVerified();
    features.push_back(
        production_features::ProductionFeature::TH_ALPM_ROUTE_SCALE);
    return features;
  }
};

TEST_F(AgentThAlpmRouteScaleTest, thAlpmScale) {
  runTest<utility::THAlpmRouteScaleGenerator>();
}

class AgentHgridDuRouteScaleTest : public AgentRouteScaleTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto features = AgentRouteScaleTest::getProductionFeaturesVerified();
    features.push_back(
        production_features::ProductionFeature::HGRID_DU_ROUTE_SCALE);
    return features;
  }
};

TEST_F(AgentHgridDuRouteScaleTest, hgridDuScaleTest) {
  runTest<utility::HgridDuRouteScaleGenerator>();
}

class AgentHgridUuRouteScaleTest : public AgentRouteScaleTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto features = AgentRouteScaleTest::getProductionFeaturesVerified();
    features.push_back(
        production_features::ProductionFeature::HGRID_UU_ROUTE_SCALE);
    return features;
  }
};

TEST_F(AgentHgridUuRouteScaleTest, hgridUuScaleTest) {
  runTest<utility::HgridUuRouteScaleGenerator>();
}

class AgentHundredThousandRouteScaleTest : public AgentRouteScaleTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto features = AgentRouteScaleTest::getProductionFeaturesVerified();
    features.push_back(
        production_features::ProductionFeature::HUNDRED_THOUSAND_ROUTE_SCALE);
    return features;
  }
};

TEST_F(AgentHundredThousandRouteScaleTest, hundredThousandRouteScaleTest) {
  runTest<utility::AnticipatedRouteScaleGenerator>();
}
} // namespace facebook::fboss
