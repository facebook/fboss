/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FabricReachabilityManager.h"
#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

namespace facebook::fboss {
class FabricReachabilityManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    // Create a separate instance of DsfSubscriber (vs
    // using one from SwSwitch) for ease of testing.
    fabricReachabilityManager_ = std::make_unique<FabricReachabilityManager>();
  }

  cfg::DsfNode makeDsfNodeCfg(int64_t switchId, std::string name) {
    cfg::DsfNode dsfNodeCfg;
    dsfNodeCfg.switchId() = switchId;
    dsfNodeCfg.name() = name;
    return dsfNodeCfg;
  }

  std::shared_ptr<DsfNode> makeDsfNode(int64_t switchId, std::string name) {
    auto dsfNode = std::make_shared<DsfNode>(SwitchID(switchId));
    auto cfgDsfNode = makeDsfNodeCfg(switchId, name);
    cfgDsfNode.asicType() = cfg::AsicType::ASIC_TYPE_RAMON;
    dsfNode->fromThrift(cfgDsfNode);
    return dsfNode;
  }

  std::shared_ptr<Port> makePort(const int portId) {
    state::PortFields portFields;
    portFields.portId() = portId;
    portFields.portName() = folly::sformat("port{}", portId);
    auto swPort = std::make_shared<Port>(std::move(portFields));
    swPort->setAdminState(cfg::PortState::ENABLED);
    swPort->setProfileId(
        cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER);
    return swPort;
  }

 protected:
  std::unique_ptr<HwTestHandle> handle_;
  std::unique_ptr<FabricReachabilityManager> fabricReachabilityManager_;
};

TEST_F(FabricReachabilityManagerTest, validateProcessReachabilityInfo) {
  std::shared_ptr<Port> swPort = makePort(1);
  cfg::PortNeighbor nbr;
  nbr.remotePort() = "fab1/2/3";
  nbr.remoteSystem() = "fdswA";

  std::map<PortID, FabricEndpoint> hwReachabilityMap;
  FabricEndpoint endpoint;

  endpoint.switchId() = 10;
  endpoint.isAttached() = true;

  hwReachabilityMap.emplace(swPort->getID(), endpoint);
  swPort->setExpectedNeighborReachability({nbr});
  auto dsfNode = makeDsfNode(10, "fdswA");

  fabricReachabilityManager_->addPort(swPort);
  fabricReachabilityManager_->addDsfNode(dsfNode);

  const auto expectedReachabilityMap =
      fabricReachabilityManager_->processReachabilityInfo(hwReachabilityMap);
  EXPECT_EQ(expectedReachabilityMap.size(), 1);

  for (const auto& expectedReachability : expectedReachabilityMap) {
    const auto& neighbor = expectedReachability.second;
    EXPECT_NE(neighbor.expectedPortId(), 0);
    EXPECT_EQ(neighbor.expectedSwitchId(), 10);
    EXPECT_EQ(neighbor.switchId(), 10);
    EXPECT_TRUE(*neighbor.isAttached());
  }
}
} // namespace facebook::fboss
