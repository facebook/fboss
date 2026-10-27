// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <gtest/gtest.h>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vrf.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

class VrfConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    platform_ = createMockPlatform();
    state_ = std::make_shared<SwitchState>();
    addSwitchInfo(state_);
    config_ = testConfigA();
  }

  static cfg::VrfConfig makeVrfConfig(
      int32_t vrfId,
      const std::optional<std::string>& name = std::nullopt) {
    cfg::VrfConfig vrf;
    vrf.vrfId() = vrfId;
    if (name) {
      vrf.name() = *name;
    }
    return vrf;
  }

  std::shared_ptr<SwitchState> apply() {
    return publishAndApplyConfig(state_, &config_, platform_.get());
  }

  std::shared_ptr<SwitchState> state_;
  std::shared_ptr<Platform> platform_;
  cfg::SwitchConfig config_;
};

TEST_F(VrfConfigTest, CreateVrfs) {
  config_.vrfs() = {makeVrfConfig(1001, "red"), makeVrfConfig(1002)};
  state_ = apply();
  ASSERT_NE(state_, nullptr);

  EXPECT_EQ(state_->getVrfs()->getNode(RouterID(1001))->getName(), "red");
  EXPECT_EQ(
      state_->getVrfs()->getNode(RouterID(1002))->getName(), std::nullopt);
}

TEST_F(VrfConfigTest, RenameVrf) {
  config_.vrfs() = {makeVrfConfig(1001, "red")};
  state_ = apply();
  ASSERT_NE(state_, nullptr);

  config_.vrfs() = {makeVrfConfig(1001, "blue")};
  state_ = apply();
  ASSERT_NE(state_, nullptr);

  EXPECT_EQ(state_->getVrfs()->getNode(RouterID(1001))->getName(), "blue");
}

TEST_F(VrfConfigTest, RemoveVrf) {
  config_.vrfs() = {makeVrfConfig(1001), makeVrfConfig(1002)};
  state_ = apply();
  ASSERT_NE(state_, nullptr);

  config_.vrfs() = {makeVrfConfig(1002)};
  state_ = apply();
  ASSERT_NE(state_, nullptr);

  EXPECT_EQ(state_->getVrfs()->getNodeIf(RouterID(1001)), nullptr);
  EXPECT_NE(state_->getVrfs()->getNodeIf(RouterID(1002)), nullptr);
}

TEST_F(VrfConfigTest, NoChange) {
  config_.vrfs() = {makeVrfConfig(1001, "red")};
  state_ = apply();
  ASSERT_NE(state_, nullptr);

  EXPECT_EQ(apply(), nullptr);
}

TEST_F(VrfConfigTest, DuplicateVrfIdRejected) {
  config_.vrfs() = {makeVrfConfig(1001), makeVrfConfig(1001)};
  EXPECT_THROW(apply(), FbossError);

  config_.vrfs() = {makeVrfConfig(1001)};
  state_ = apply();
  ASSERT_NE(state_, nullptr);
  config_.vrfs() = {makeVrfConfig(1001), makeVrfConfig(1001)};
  EXPECT_THROW(apply(), FbossError);
}

TEST_F(VrfConfigTest, DefaultVrfIdRejected) {
  config_.vrfs() = {makeVrfConfig(0)};
  EXPECT_THROW(apply(), FbossError);
}

TEST_F(VrfConfigTest, NegativeVrfIdRejected) {
  config_.vrfs() = {makeVrfConfig(-1)};
  EXPECT_THROW(apply(), FbossError);
}

TEST(VrfResource, MaxVrfs) {
  // Mock ASIC supports 8 VRFs, including the default VRF
  auto config = testConfigA();
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();
  for (int32_t id = 1; id <= 7; ++id) {
    config.vrfs().ensure().emplace_back().vrfId() = id;
  }
  sw->applyConfig("7 vrfs", config);
  EXPECT_EQ(sw->getState()->getVrfs()->numNodes(), 7);

  config.vrfs()->emplace_back().vrfId() = 8;
  EXPECT_THROW(sw->applyConfig("8 vrfs", config), FbossError);
  EXPECT_EQ(sw->getState()->getVrfs()->numNodes(), 7);
}
