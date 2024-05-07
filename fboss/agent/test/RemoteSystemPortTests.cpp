// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

class RemoteSystemPortTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_multi_switch = true;
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    waitForStateUpdates(sw_);
  }

  cfg::SwitchConfig initialConfig() const {
    return testConfigB();
  }

  void TearDown() override {
    sw_->stop(false, false);
  }

  void verify(int expectedSize = 2) {
    auto state = sw_->getState();
    auto local = state->getSystemPorts();
    auto remote = state->getRemoteSystemPorts();

    EXPECT_EQ(remote->size(), expectedSize);

    auto systemPorts1 = state->getSystemPorts()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(1)}));
    auto systemPorts2 = state->getSystemPorts()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(2)}));

    auto remoteSystemPorts1 = state->getRemoteSystemPorts()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(1)}));
    auto remoteSystemPorts2 = state->getRemoteSystemPorts()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(2)}));

    EXPECT_EQ(systemPorts1->toThrift(), remoteSystemPorts2->toThrift());
    EXPECT_EQ(systemPorts2->toThrift(), remoteSystemPorts1->toThrift());
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(RemoteSystemPortTest, ConfigureSystemPorts) {
  verify();
}

TEST_F(RemoteSystemPortTest, ReConfigureSystemPorts) {
  auto matchAll = sw_->getScopeResolver()->scope(cfg::SwitchType::VOQ);
  ;
  sw_->updateStateBlocking(
      "Add System Ports", [=](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        auto remoteSysPorts = state->getRemoteSystemPorts()->modify(&newState);
        remoteSysPorts->addNode(
            std::make_shared<SystemPort>(SystemPortID(512)), matchAll);
        remoteSysPorts->addNode(
            std::make_shared<SystemPort>(SystemPortID(513)), matchAll);
        return newState;
      });
  auto allRemoteSysPorts0 =
      sw_->getState()->getRemoteSystemPorts()->getMapNodeIf(matchAll);
  EXPECT_NE(nullptr, allRemoteSysPorts0);
  auto newCfg = initialConfig();
  for (auto port : *newCfg.ports()) {
    port.minFrameSize() = 1024;
  }
  sw_->applyConfig("Reconfigure", newCfg);
  verify(3);
  auto allRemoteSysPorts1 =
      sw_->getState()->getRemoteSystemPorts()->getMapNodeIf(matchAll);
  EXPECT_NE(nullptr, allRemoteSysPorts1);
  EXPECT_EQ(allRemoteSysPorts0, allRemoteSysPorts1);
}

} // namespace facebook::fboss
