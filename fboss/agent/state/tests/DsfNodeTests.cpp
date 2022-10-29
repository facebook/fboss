/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

std::vector<std::string> getLoopbackIps(int64_t switchIdVal) {
  CHECK_LT(switchIdVal, 10) << " Switch Id >= 10, not supported";

  auto v6 = folly::sformat("20{}::1/64", switchIdVal);
  auto v4 = folly::sformat("20{}.0.0.1/24", switchIdVal);
  return {v6, v4};
}

std::shared_ptr<DsfNode> makeDsfNode(
    int64_t switchId = 0,
    cfg::DsfNodeType type = cfg::DsfNodeType::INTERFACE_NODE) {
  cfg::DsfNode dsfNodeCfg;
  dsfNodeCfg.switchId() = switchId;
  dsfNodeCfg.name() = folly::sformat("dsfNodeCfg{}", switchId);
  dsfNodeCfg.type() = type;
  const auto kBlockSize{100};
  dsfNodeCfg.systemPortRange()->minimum() = switchId * kBlockSize;
  dsfNodeCfg.systemPortRange()->maximum() = switchId * kBlockSize + kBlockSize;
  dsfNodeCfg.loopbackIps() = getLoopbackIps(switchId);
  auto dsfNode = std::make_shared<DsfNode>(SwitchID(switchId));
  dsfNode->fromThrift(dsfNodeCfg);
  return dsfNode;
}

TEST(DsfNode, SerDeserDsfNode) {
  auto dsfNode = makeDsfNode();
  auto serialized = dsfNode->toFollyDynamic();
  auto dsfNodeBack = DsfNode::fromFollyDynamic(serialized);
  EXPECT_TRUE(*dsfNode == *dsfNodeBack);
}

TEST(DsfNode, SerDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  auto dsfNode1 = makeDsfNode(1);
  auto dsfNode2 = makeDsfNode(2);

  auto dsfNodeMap = std::make_shared<DsfNodeMap>();
  dsfNodeMap->addNode(dsfNode1);
  dsfNodeMap->addDsfNode(dsfNode2);
  state->resetDsfNodes(dsfNodeMap);

  auto serialized = state->toFollyDynamic();
  auto stateBack = SwitchState::fromFollyDynamic(serialized);

  // Check all dsfNodes should be there
  for (auto switchID : {SwitchID(1), SwitchID(2)}) {
    EXPECT_TRUE(
        *state->getDsfNodes()->getDsfNodeIf(switchID) ==
        *stateBack->getDsfNodes()->getDsfNodeIf(switchID));
  }
  EXPECT_EQ(state->getDsfNodes()->size(), 2);
}

TEST(DsfNode, addRemove) {
  auto state = std::make_shared<SwitchState>();
  auto dsfNode1 = makeDsfNode(1);
  auto dsfNode2 = makeDsfNode(2);
  state->getDsfNodes()->addNode(dsfNode1);
  state->getDsfNodes()->addNode(dsfNode2);
  EXPECT_EQ(state->getDsfNodes()->size(), 2);

  state->getDsfNodes()->removeNode(1);
  EXPECT_EQ(state->getDsfNodes()->size(), 1);
  EXPECT_EQ(state->getDsfNodes()->getNodeIf(1), nullptr);
  EXPECT_NE(state->getDsfNodes()->getNodeIf(2), nullptr);
}

TEST(DsfNode, update) {
  auto state = std::make_shared<SwitchState>();
  auto dsfNode = makeDsfNode(1);
  state->getDsfNodes()->addNode(dsfNode);
  EXPECT_EQ(state->getDsfNodes()->size(), 1);
  auto newDsfNode = makeDsfNode(1, cfg::DsfNodeType::FABRIC_NODE);
  state->getDsfNodes()->updateNode(newDsfNode);

  EXPECT_NE(*dsfNode, *state->getDsfNodes()->getNode(1));
}
