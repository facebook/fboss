// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/dynamic.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

class RemoteSystemInterfaceTest : public ::testing::Test {
 public:
  void SetUp() override {
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
    auto local = state->getInterfaces();
    auto remote = state->getRemoteInterfaces();

    EXPECT_EQ(remote->size(), expectedSize);

    auto intfs1 = state->getInterfaces()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(1)}));
    auto intfs2 = state->getInterfaces()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(2)}));

    auto remoteIntfs1 = state->getRemoteInterfaces()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(1)}));
    auto remoteIntfs2 = state->getRemoteInterfaces()->getMapNodeIf(
        HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(2)}));

    EXPECT_EQ(remoteIntfs1->toThrift(), intfs2->toThrift());
    EXPECT_EQ(remoteIntfs2->toThrift(), intfs1->toThrift());
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(RemoteSystemInterfaceTest, ConfigureSystemPortInterfaces) {
  verify();
}

TEST_F(RemoteSystemInterfaceTest, ReConfigureSystemPortInterfaces) {
  auto matchAll = sw_->getScopeResolver()->scope(cfg::SwitchType::VOQ);
  ;
  sw_->updateStateBlocking(
      "Add System Interface", [=](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        auto remoteSysIntfs = state->getRemoteInterfaces()->modify(&newState);
        remoteSysIntfs->addNode(
            std::make_shared<Interface>(
                InterfaceID(512),
                RouterID(0),
                std::optional<VlanID>(std::nullopt),
                folly::StringPiece("512"),
                folly::MacAddress("00:02:00:00:00:01"),
                9000,
                true,
                true,
                cfg::InterfaceType::SYSTEM_PORT),
            matchAll);
        remoteSysIntfs->addNode(
            std::make_shared<Interface>(
                InterfaceID(513),
                RouterID(0),
                std::optional<VlanID>(std::nullopt),
                folly::StringPiece("513"),
                folly::MacAddress("00:02:00:00:00:02"),
                9000,
                true,
                true,
                cfg::InterfaceType::SYSTEM_PORT),
            matchAll);
        return newState;
      });

  verify(3);
  auto allRemoteSysIntfs0 =
      sw_->getState()->getRemoteInterfaces()->getMapNodeIf(matchAll);
  EXPECT_NE(nullptr, allRemoteSysIntfs0);
  EXPECT_NE(0, allRemoteSysIntfs0->size());
  auto newCfg = initialConfig();
  for (auto port : *newCfg.ports()) {
    port.minFrameSize() = 1024;
  }
  sw_->applyConfig("Reconfigure", newCfg);
  verify(3);
  auto allRemoteSysIntfs1 =
      sw_->getState()->getRemoteInterfaces()->getMapNodeIf(matchAll);
  EXPECT_NE(nullptr, allRemoteSysIntfs1);
  EXPECT_EQ(allRemoteSysIntfs0, allRemoteSysIntfs1);
}

} // namespace facebook::fboss
