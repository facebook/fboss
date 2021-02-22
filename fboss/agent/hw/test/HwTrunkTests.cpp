// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestTrunkUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace facebook::fboss {
class HwTrunkTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& switchCfg) {
    auto state = applyNewConfig(switchCfg);
    applyNewState(utility::enableTrunkPorts(state));
  }
};

TEST_F(HwTrunkTest, TrunkCreateHighLowKeyIds) {
  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(
        std::numeric_limits<AggregatePortID>::max(),
        {masterLogicalPortIds()[0]},
        &cfg);
    utility::addAggPort(1, {masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=]() {
    utility::verifyAggregatePortCount(getHwSwitchEnsemble(), 2);
    utility::verifyAggregatePort(
        getHwSwitchEnsemble(), AggregatePortID(1)

    );
    utility::verifyAggregatePort(
        getHwSwitchEnsemble(),
        AggregatePortID(std::numeric_limits<AggregatePortID>::max())

    );
    auto aggIDs = {
        AggregatePortID(1),
        AggregatePortID(std::numeric_limits<AggregatePortID>::max())};
    for (auto aggId : aggIDs) {
      utility::verifyAggregatePortMemberCount(
          getHwSwitchEnsemble(), aggId, 1, 1);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTrunkTest, TrunkCheckIngressPktAggPort) {
  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(
        std::numeric_limits<AggregatePortID>::max(),
        {masterLogicalPortIds()[0]},
        &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=]() {
    utility::verifyPktFromAggregatePort(
        getHwSwitchEnsemble(),
        AggregatePortID(std::numeric_limits<AggregatePortID>::max()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTrunkTest, TrunkMemberPortDownMinLinksViolated) {
  auto aggId = AggregatePortID(std::numeric_limits<AggregatePortID>::max());

  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(
        aggId, {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);

    bringDownPort(PortID(masterLogicalPortIds()[0]));
    // Member port count should drop to 1 now.
  };
  auto verify = [=]() {
    utility::verifyAggregatePortCount(getHwSwitchEnsemble(), 1);
    utility::verifyAggregatePort(getHwSwitchEnsemble(), aggId);
    utility::verifyAggregatePortMemberCount(getHwSwitchEnsemble(), aggId, 2, 1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
