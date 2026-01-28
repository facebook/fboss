// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ThriftBasedWarmbootUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/test/TestUtils.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

namespace facebook::fboss {

class ThriftBasedWarmbootUtilsTest : public ::testing::Test {
 public:
  void SetUp() override {
    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
    cfg::SwitchInfo switchInfo;
    switchInfo.switchIndex() = 0;
    switchInfo.switchType() = cfg::SwitchType::NPU;
    switchInfo.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
    switchIdToSwitchInfo[0] = switchInfo;

    testThriftClientTable_ =
        std::make_unique<HwSwitchThriftClientTableForTesting>(
            0, switchIdToSwitchInfo);

    asicTable_ = std::make_unique<HwAsicTable>(
        switchIdToSwitchInfo, std::nullopt, std::map<int64_t, cfg::DsfNode>());
  }

  state::SwitchState createSwitchStateWithPorts() {
    state::SwitchState state;
    state::PortFields port1;
    port1.portId() = 1;
    port1.portName() = "port1";
    port1.portOperState() = false;

    state::PortFields port2;
    port2.portId() = 2;
    port2.portName() = "port2";
    port2.portOperState() = true;

    std::map<int16_t, state::PortFields> portMap;
    portMap[1] = port1;
    portMap[2] = port2;
    state.portMaps()["id=0"] = portMap;

    return state;
  }

 protected:
  std::unique_ptr<HwSwitchThriftClientTableForTesting> testThriftClientTable_;
  std::unique_ptr<HwAsicTable> asicTable_;
};

// ============================================================================
// NEGATIVE TEST CASES - Conditions that should return nullopt
// ============================================================================

TEST_F(
    ThriftBasedWarmbootUtilsTest,
    CheckAndGetWarmbootStateFailsWhenNotMultiSwitch) {
  // When not in multi-switch mode, warmboot from HwSwitch should not be
  // attempted
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);

  auto result = checkAndGetWarmbootStateFromHwSwitch(
      false, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_FALSE(result.has_value());
}

TEST_F(ThriftBasedWarmbootUtilsTest, CheckAndGetWarmbootStateMultipleSwitches) {
  // Create a setup with multiple switches
  std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
  for (int i = 0; i < 2; i++) {
    cfg::SwitchInfo switchInfo;
    switchInfo.switchIndex() = i;
    switchInfo.switchType() = cfg::SwitchType::NPU;
    switchInfo.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
    switchIdToSwitchInfo[i] = switchInfo;
  }

  auto multiSwitchAsicTable = std::make_unique<HwAsicTable>(
      switchIdToSwitchInfo, std::nullopt, std::map<int64_t, cfg::DsfNode>());

  // Warmboot from multiple HW Agents is not supported yet.
  // TODO(zecheng): Update this UT when multi HW Agent is supported.
  auto result = checkAndGetWarmbootStateFromHwSwitch(
      true, multiSwitchAsicTable.get(), testThriftClientTable_.get());

  EXPECT_FALSE(result.has_value());
}

TEST_F(
    ThriftBasedWarmbootUtilsTest,
    CheckAndGetWarmbootStateFailsWhenNotConfigured) {
  // When switch is in INITIALIZED state (not CONFIGURED), warmboot should fail
  testThriftClientTable_->setRunState(SwitchRunState::INITIALIZED);

  auto result = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_FALSE(result.has_value());
}

TEST_F(
    ThriftBasedWarmbootUtilsTest,
    CheckAndGetWarmbootStateFailsWhenUninitialized) {
  // When switch is in UNINITIALIZED state, warmboot should fail
  testThriftClientTable_->setRunState(SwitchRunState::UNINITIALIZED);

  auto result = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_FALSE(result.has_value());
}

TEST_F(ThriftBasedWarmbootUtilsTest, CheckAndGetWarmbootStateFailsWhenExiting) {
  // When switch is in EXITING state, warmboot should fail
  testThriftClientTable_->setRunState(SwitchRunState::EXITING);

  auto result = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_FALSE(result.has_value());
}

// ============================================================================
// EXCEPTION HANDLING TEST CASES
// ============================================================================

TEST_F(
    ThriftBasedWarmbootUtilsTest,
    CheckAndGetWarmbootStateHandlesGetRunStateException) {
  // When getting run state throws an exception, the function should catch it
  // and return nullopt
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);
  testThriftClientTable_->setShouldThrowOnGetRunState(true);

  auto result = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_FALSE(result.has_value());
}

TEST_F(
    ThriftBasedWarmbootUtilsTest,
    CheckAndGetWarmbootStateHandlesGetProgrammedStateException) {
  // When switch is CONFIGURED but getting programmed state throws an exception,
  // the function should catch it and return nullopt
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);
  testThriftClientTable_->setShouldThrowOnGetProgrammedState(true);

  auto result = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_FALSE(result.has_value());
}

// ============================================================================
// SUCCESS TEST CASES
// ============================================================================

TEST_F(
    ThriftBasedWarmbootUtilsTest,
    CheckAndGetWarmbootStateSuccessWithConfiguredState) {
  // When switch is in CONFIGURED state and programmed state can be
  // retrieved, warmboot should succeed
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);
  auto switchState = createSwitchStateWithPorts();
  testThriftClientTable_->setProgrammedState(switchState);

  auto result = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value().swSwitchState(), switchState);
}

TEST_F(ThriftBasedWarmbootUtilsTest, MultipleCallsToCheckAndGetWarmbootState) {
  // Multiple calls should consistently return warmboot state
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);
  auto switchState = createSwitchStateWithPorts();
  testThriftClientTable_->setProgrammedState(switchState);

  auto result1 = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());
  auto result2 = checkAndGetWarmbootStateFromHwSwitch(
      true, asicTable_.get(), testThriftClientTable_.get());

  EXPECT_TRUE(result1.has_value());
  EXPECT_TRUE(result2.has_value());
  EXPECT_EQ(result1.value().swSwitchState(), switchState);
  EXPECT_EQ(result2.value().swSwitchState(), switchState);
}

} // namespace facebook::fboss
