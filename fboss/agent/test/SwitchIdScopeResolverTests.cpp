// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class SwitchIdScopeResolverTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
  }
  HwSwitchMatcher l3SwitchMatcher() const {
    return HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID{1}});
  }
  const SwitchIdScopeResolver& scopeResolver() const {
    return *sw_->getScopeResolver();
  }
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(SwitchIdScopeResolverTest, mirrorScope) {
  EXPECT_EQ(l3SwitchMatcher(), scopeResolver().scope(cfg::Mirror{}));
  EXPECT_EQ(
      l3SwitchMatcher(), scopeResolver().scope(std::shared_ptr<Mirror>()));
}

TEST_F(SwitchIdScopeResolverTest, dsfNodeScope) {
  EXPECT_EQ(l3SwitchMatcher(), scopeResolver().scope(cfg::DsfNode{}));
  EXPECT_EQ(
      l3SwitchMatcher(), scopeResolver().scope(std::shared_ptr<DsfNode>()));
}

TEST_F(SwitchIdScopeResolverTest, portScope) {
  EXPECT_EQ(l3SwitchMatcher(), scopeResolver().scope(PortID(6)));
}

TEST_F(SwitchIdScopeResolverTest, aggPortScope) {
  cfg::AggregatePort aggPort;
  aggPort.name() = "agg";
  cfg::AggregatePortMember member;
  member.memberPortID() = 6;
  aggPort.memberPorts()->push_back(member);
  EXPECT_EQ(l3SwitchMatcher(), scopeResolver().scope(aggPort));
}

TEST_F(SwitchIdScopeResolverTest, sysPortScope) {
  EXPECT_EQ(l3SwitchMatcher(), scopeResolver().scope(SystemPortID(101)));
  EXPECT_EQ(
      l3SwitchMatcher(),
      scopeResolver().scope(std::make_shared<SystemPort>(SystemPortID(101))));
  EXPECT_THROW(scopeResolver().scope(SystemPortID(1001)), FbossError);
  EXPECT_THROW(
      scopeResolver().scope(std::make_shared<SystemPort>(SystemPortID(1001))),
      FbossError);
}
