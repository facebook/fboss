// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class HwFabricSwitchTest : public HwLinkStateDependentTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        false /*interfaceHasSubnet*/,
        false, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/
    );
    utility::populatePortExpectedNeighbors(masterLogicalPortIds(), cfg);
    return cfg;
  }
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::FABRIC);
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

TEST_F(HwFabricSwitchTest, reachDiscard) {
  auto verify = [this]() {
    getHwSwitch()->updateStats();
    auto beforeSwitchDrops = getHwSwitch()->getSwitchDropStats();
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        "TX 1 destination=-1 destinationModid=-1 flags=0x8000\n", out);
    getHwSwitchEnsemble()->runDiagCommand("quit\n", out);
    WITH_RETRIES({
      getHwSwitch()->updateStats();
      auto afterSwitchDrops = getHwSwitch()->getSwitchDropStats();
      XLOG(INFO) << " Before reach drops: "
                 << *beforeSwitchDrops.globalReachabilityDrops()
                 << " After reach drops: "
                 << *afterSwitchDrops.globalReachabilityDrops()
                 << " Before global drops: " << *beforeSwitchDrops.globalDrops()
                 << " After global drops: : "
                 << *afterSwitchDrops.globalDrops();
      EXPECT_EVENTUALLY_EQ(
          *afterSwitchDrops.globalReachabilityDrops(),
          *beforeSwitchDrops.globalReachabilityDrops() + 1);
      // Global drops are in bytes
      EXPECT_EVENTUALLY_GT(
          *afterSwitchDrops.globalDrops(), *beforeSwitchDrops.globalDrops());
    });
    checkNoStatsChange();
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss
