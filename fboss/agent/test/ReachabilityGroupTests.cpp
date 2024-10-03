// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

namespace {
constexpr auto kParallelLinkPerNode = 4;
constexpr auto kDualStageLevel2FabricNodes = 2;
} // namespace

namespace facebook::fboss {

class ReachabilityGroupTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_multi_switch = true;
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    waitForStateUpdates(sw_);
  }

  virtual cfg::SwitchConfig initialConfig() const {
    return testConfigFabricSwitch(
        false /* dualStage*/, 1 /* fabricLevel */, kParallelLinkPerNode);
  }

  void verifyReachabilityGroupListSize(int expectedSize) const {
    const auto& state = sw_->getState();
    for (const auto& [_, switchSettings] :
         std::as_const(*state->getSwitchSettings())) {
      if (expectedSize > 0) {
        EXPECT_TRUE(switchSettings->getReachabilityGroupListSize().has_value());
        EXPECT_EQ(
            switchSettings->getReachabilityGroupListSize().value(),
            expectedSize);
      } else {
        EXPECT_FALSE(
            switchSettings->getReachabilityGroupListSize().has_value());
      }
    }
  }

  int numIntfNode(const cfg::SwitchConfig& config) const {
    int count = 0;
    for (const auto& [_, node] : *config.dsfNodes()) {
      if (*node.type() == cfg::DsfNodeType::INTERFACE_NODE) {
        count++;
      }
    }
    return count;
  }

  int numClusterIds(const cfg::SwitchConfig& config) const {
    std::unordered_set<int> clusterIds;
    for (const auto& [_, node] : *config.dsfNodes()) {
      if (node.clusterId().has_value()) {
        clusterIds.insert(node.clusterId().value());
      }
    }
    return clusterIds.size();
  }

  cfg::DsfNodeType getPortNeighborDsfNodeType(
      const cfg::SwitchConfig& config,
      std::shared_ptr<Port> port) const {
    CHECK(!port->getExpectedNeighborValues()->empty());
    std::string expectedNeighborName = *port->getExpectedNeighborValues()
                                            ->cbegin()
                                            ->get()
                                            ->toThrift()
                                            .remoteSystem();
    for (const auto& [_, node] : *config.dsfNodes()) {
      if (*node.name() == expectedNeighborName) {
        return *node.type();
      }
    }
    throw FbossError("No DSF node found with name ", expectedNeighborName);
  }

  void TearDown() override {
    sw_->stop(false, false);
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

class ReachabilityGroupSingleStageFdswTest : public ReachabilityGroupTest {};

class ReachabilityGroupSingleStageFdswNoParallelIntfLinkTest
    : public ReachabilityGroupTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    return testConfigFabricSwitch(
        false /* dualStage*/, 1 /* fabricLevel */, 1 /* parallLinkPerNode */);
  }
};

class ReachabilityGroupDualStageFdswNoParallelIntfLinkTest
    : public ReachabilityGroupTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    return testConfigFabricSwitch(
        true /* dualStage*/,
        1 /* fabricLevel */,
        1 /* parallLinkPerNode */,
        kDualStageLevel2FabricNodes);
  }
};

class ReachabilityGroupDualStageFdswTest : public ReachabilityGroupTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    return testConfigFabricSwitch(
        true /* dualStage*/,
        1 /* fabricLevel */,
        kParallelLinkPerNode,
        kDualStageLevel2FabricNodes);
  }
};

class ReachabilityGroupDualStageSdswTest : public ReachabilityGroupTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    return testConfigFabricSwitch(true /* dualStage*/, 2 /* fabricLevel */);
  }
};

TEST_F(ReachabilityGroupSingleStageFdswTest, test) {
  // Single stage FDSW with parallel links to interface node.
  // There should be one group per interface node.
  verifyReachabilityGroupListSize(numIntfNode(initialConfig()));

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      EXPECT_TRUE(port->getReachabilityGroupId().has_value());
      EXPECT_EQ(
          port->getReachabilityGroupId().value(),
          ((port->getID() - 1) / kParallelLinkPerNode) + 1);
    }
  }
}

TEST_F(ReachabilityGroupSingleStageFdswNoParallelIntfLinkTest, test) {
  // Single stage FDSW with no parallel links to interface node.
  // There should be no reachability group programming.
  verifyReachabilityGroupListSize(0);

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      EXPECT_FALSE(port->getReachabilityGroupId().has_value());
    }
  }
}

TEST_F(ReachabilityGroupDualStageFdswNoParallelIntfLinkTest, test) {
  // Dual stage FDSW with no parallel links to interface node.
  // One group for uplinks to SDSW (1) and one group for downlink to RDSW (2).
  verifyReachabilityGroupListSize(2);

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      auto neighborDsfNodeType =
          getPortNeighborDsfNodeType(initialConfig(), port);

      EXPECT_TRUE(port->getReachabilityGroupId().has_value());
      EXPECT_EQ(
          port->getReachabilityGroupId().value(),
          neighborDsfNodeType == cfg::DsfNodeType::FABRIC_NODE ? 1 : 2);
    }
  }
}

TEST_F(ReachabilityGroupDualStageFdswTest, test) {
  // Dual stage FDSW with parallel links to interface node.
  // One group for uplinks to SDSW (1) and one group per interface node.
  verifyReachabilityGroupListSize(
      initialConfig().dsfNodes()->size() - kDualStageLevel2FabricNodes);

  std::unordered_map<std::string, int> remoteSystem2ReachabilityGroup;

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      EXPECT_TRUE(port->getReachabilityGroupId().has_value());
      auto neighborDsfNodeType =
          getPortNeighborDsfNodeType(initialConfig(), port);

      if (neighborDsfNodeType == cfg::DsfNodeType::FABRIC_NODE) {
        EXPECT_EQ(port->getReachabilityGroupId().value(), 1);
      } else {
        CHECK(!port->getExpectedNeighborValues()->empty());
        std::string expectedNeighborName = *port->getExpectedNeighborValues()
                                                ->cbegin()
                                                ->get()
                                                ->toThrift()
                                                .remoteSystem();
        // Offset 2 because 1) reachability group starts at 1, and 2) fabric
        // links take group 1 already.
        auto [it, _] = remoteSystem2ReachabilityGroup.insert(
            {expectedNeighborName, remoteSystem2ReachabilityGroup.size() + 2});
        EXPECT_EQ(port->getReachabilityGroupId().value(), it->second);
      }
    }
  }
}

TEST_F(ReachabilityGroupDualStageSdswTest, test) {
  // Dual stage SDSW with parallel links to FDSW.
  // There should be one group per cluster ID.
  verifyReachabilityGroupListSize(numClusterIds(initialConfig()));

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      EXPECT_TRUE(port->getReachabilityGroupId().has_value());
      EXPECT_EQ(
          port->getReachabilityGroupId().value(),
          ((port->getID() - 1) / kParallelLinkPerNode) + 1);
    }
  }
}

TEST_F(
    ReachabilityGroupDualStageFdswNoParallelIntfLinkTest,
    configUpdateToParallelIntfLinks) {
  // Dual stage FDSW with no parallel links to interface node.
  // One group for uplinks to SDSW (1) and one group for downlink to RDSW (2).
  verifyReachabilityGroupListSize(2);

  const auto newConfig = testConfigFabricSwitch(
      true /* dualStage*/,
      1 /* fabricLevel */,
      kParallelLinkPerNode,
      kDualStageLevel2FabricNodes);

  sw_->applyConfig("New config to parallel intf links", newConfig);

  // Dual stage FDSW with parallel links to interface node.
  // One group for uplinks to SDSW (1) and one group per interface node.
  verifyReachabilityGroupListSize(
      newConfig.dsfNodes()->size() - kDualStageLevel2FabricNodes);

  std::unordered_map<std::string, int> remoteSystem2ReachabilityGroup;

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      EXPECT_TRUE(port->getReachabilityGroupId().has_value());
      auto neighborDsfNodeType = getPortNeighborDsfNodeType(newConfig, port);

      if (neighborDsfNodeType == cfg::DsfNodeType::FABRIC_NODE) {
        EXPECT_EQ(port->getReachabilityGroupId().value(), 1);
      } else {
        CHECK(!port->getExpectedNeighborValues()->empty());
        std::string expectedNeighborName = *port->getExpectedNeighborValues()
                                                ->cbegin()
                                                ->get()
                                                ->toThrift()
                                                .remoteSystem();
        // Offset 2 because 1) reachability group starts at 1, and 2) fabric
        // links take group 1 already.
        auto [it, _] = remoteSystem2ReachabilityGroup.insert(
            {expectedNeighborName, remoteSystem2ReachabilityGroup.size() + 2});
        EXPECT_EQ(port->getReachabilityGroupId().value(), it->second);
      }
    }
  }
}

TEST_F(
    ReachabilityGroupSingleStageFdswNoParallelIntfLinkTest,
    configUpdateToParallelIntfLinks) {
  // Single stage FDSW with no parallel links to interface node.
  // There should be no reachability group programming.
  verifyReachabilityGroupListSize(0);

  const auto newConfig = testConfigFabricSwitch(
      false /* dualStage*/, 1 /* fabricLevel */, kParallelLinkPerNode);

  sw_->applyConfig("New config to parallel intf links", newConfig);

  // Single stage FDSW with parallel links to interface node.
  // There should be one group per interface node.
  verifyReachabilityGroupListSize(numIntfNode(newConfig));

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      EXPECT_TRUE(port->getReachabilityGroupId().has_value());
      EXPECT_EQ(
          port->getReachabilityGroupId().value(),
          ((port->getID() - 1) / kParallelLinkPerNode) + 1);
    }
  }
}

TEST_F(ReachabilityGroupSingleStageFdswTest, changeNumParallelLinks) {
  // Single stage FDSW with parallel links to interface node.
  // There should be one group per interface node.
  verifyReachabilityGroupListSize(numIntfNode(initialConfig()));

  const auto newParallelLinkPerNode = kParallelLinkPerNode / 2;
  CHECK_GT(newParallelLinkPerNode, 1);
  const auto newConfig = testConfigFabricSwitch(
      false /* dualStage*/, 1 /* fabricLevel */, newParallelLinkPerNode);

  sw_->applyConfig("New config to parallel intf links", newConfig);

  for (const auto& [_, portMap] : std::as_const(*sw_->getState()->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      EXPECT_TRUE(port->getReachabilityGroupId().has_value());
      EXPECT_EQ(
          port->getReachabilityGroupId().value(),
          ((port->getID() - 1) / newParallelLinkPerNode) + 1);
    }
  }
}

} // namespace facebook::fboss
