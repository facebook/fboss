// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/ShelManager.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"

namespace {
constexpr auto kLocalSysPortRange = 20;
}

namespace facebook::fboss {

class ShelManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    shelManager_ = std::make_unique<ShelManager>();
  }

  std::shared_ptr<SwitchState> switchStateWithLocalSysPorts(
      int localSysPortMax) {
    std::shared_ptr<MultiSwitchSystemPortMap> multiSwitchSysPorts =
        std::make_shared<MultiSwitchSystemPortMap>();
    for (int i = 0; i < localSysPortMax; i++) {
      auto sysPort = std::make_shared<SystemPort>(SystemPortID(i + 1));
      multiSwitchSysPorts->addNode(sysPort, HwSwitchMatcher());
    }
    std::shared_ptr<SwitchState> switchState = std::make_shared<SwitchState>();
    switchState->resetSystemPorts(multiSwitchSysPorts);
    return switchState;
  }

 protected:
  std::unique_ptr<ShelManager> shelManager_;
};

TEST_F(ShelManagerTest, RefCountAndIntf2AddDel) {
  auto ecmpWidth = 4;
  auto ecmpWeight = 1;
  std::vector<ResolvedNextHop> ecmpNexthops;
  ecmpNexthops.reserve(ecmpWidth);
  for (int i = 0; i < ecmpWidth; i++) {
    ecmpNexthops.emplace_back(
        folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
        InterfaceID(i + 1),
        ecmpWeight);
  }
  RouteNextHopSet ecmpNextHopSet(ecmpNexthops.begin(), ecmpNexthops.end());
  auto switchState = switchStateWithLocalSysPorts(kLocalSysPortRange);

  shelManager_->updateRefCount(ecmpNextHopSet, switchState, true /*add*/);
  EXPECT_EQ(shelManager_->intf2RefCnt_.size(), ecmpWidth);
  for (int i = 0; i < ecmpWidth; i++) {
    EXPECT_EQ(shelManager_->intf2RefCnt_.at(InterfaceID(i + 1)), 1);
  }

  shelManager_->updateRefCount(ecmpNextHopSet, switchState, false /*add*/);
  EXPECT_TRUE(shelManager_->intf2RefCnt_.empty());
}
} // namespace facebook::fboss
