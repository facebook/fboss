/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(10)}};
}

std::shared_ptr<DsfNode> makeDsfNode(
    int64_t switchId = 0,
    cfg::DsfNodeType type = cfg::DsfNodeType::INTERFACE_NODE) {
  auto dsfNode = std::make_shared<DsfNode>(SwitchID(switchId));
  dsfNode->fromThrift(makeDsfNodeCfg(switchId, type));
  return dsfNode;
}

std::vector<int64_t> getStaticNbrTimestamps(
    const std::shared_ptr<MultiSwitchInterfaceMap>& mIntfs) {
  std::vector<int64_t> timestamps;
  auto fillTimeStamps = [&timestamps](const auto& nbrTable) {
    for (const auto& [_, nbrEntry] : std::as_const(*nbrTable)) {
      if (nbrEntry->getType() == state::NeighborEntryType::STATIC_ENTRY) {
        EXPECT_TRUE(nbrEntry->getResolvedSince().has_value());
        timestamps.push_back(nbrEntry->getResolvedSince().value());
      }
    }
  };
  for (const auto& [_, intfMap] : std::as_const(*mIntfs)) {
    for (const auto& [_, interface] : std::as_const(*intfMap)) {
      fillTimeStamps(interface->getNdpTable());
      fillTimeStamps(interface->getArpTable());
    }
  }
  return timestamps;
}

TEST(DsfNode, SerDeserDsfNode) {
  auto dsfNode = makeDsfNode();
  auto serialized = dsfNode->toThrift();
  auto dsfNodeBack = std::make_shared<DsfNode>(serialized);
  EXPECT_TRUE(*dsfNode == *dsfNodeBack);
}

TEST(DsfNode, SerDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  auto dsfNode1 = makeDsfNode(1);
  auto dsfNode2 = makeDsfNode(2);

  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode1, scope());
  dsfNodeMap->addNode(dsfNode2, scope());
  state->resetDsfNodes(dsfNodeMap);

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);

  // Check all dsfNodes should be there
  for (auto switchID : {SwitchID(1), SwitchID(2)}) {
    EXPECT_TRUE(
        *state->getDsfNodes()->getNodeIf(switchID) ==
        *stateBack->getDsfNodes()->getNodeIf(switchID));
  }
  EXPECT_EQ(state->getDsfNodes()->numNodes(), 2);
}

TEST(DsfNode, addRemove) {
  auto state = std::make_shared<SwitchState>();
  auto dsfNode1 = makeDsfNode(1);
  auto dsfNode2 = makeDsfNode(2);
  state->getDsfNodes()->addNode(dsfNode1, scope());
  state->getDsfNodes()->addNode(dsfNode2, scope());
  EXPECT_EQ(state->getDsfNodes()->numNodes(), 2);

  state->getDsfNodes()->removeNode(SwitchID(1));
  EXPECT_EQ(state->getDsfNodes()->numNodes(), 1);
  EXPECT_EQ(state->getDsfNodes()->getNodeIf(SwitchID(1)), nullptr);
  EXPECT_NE(state->getDsfNodes()->getNodeIf(SwitchID(2)), nullptr);
}

TEST(DsfNode, update) {
  auto state = std::make_shared<SwitchState>();
  auto dsfNode = makeDsfNode(1);
  state->getDsfNodes()->addNode(dsfNode, scope());
  EXPECT_EQ(state->getDsfNodes()->numNodes(), 1);
  auto newDsfNode = makeDsfNode(1, cfg::DsfNodeType::FABRIC_NODE);
  state->getDsfNodes()->updateNode(newDsfNode, scope());

  EXPECT_NE(*dsfNode, *state->getDsfNodes()->getNodeIf(SwitchID(1)));
}

TEST(DsfNode, publish) {
  auto state = std::make_shared<SwitchState>();
  state->publish();
  EXPECT_TRUE(state->getDsfNodes()->isPublished());
}

TEST(DsfNode, dsfNodeApplyConfig) {
  auto platform = createMockPlatform(cfg::SwitchType::VOQ, kVoqSwitchIdBegin);
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(stateV1->getDsfNodes()->numNodes(), 2);
  // No remote neighbor
  EXPECT_EQ(getStaticNbrTimestamps(stateV1->getRemoteInterfaces()).size(), 0);
  // Have local static nbrs
  auto localStaticNbrs = getStaticNbrTimestamps(stateV1->getInterfaces());
  EXPECT_EQ(localStaticNbrs.size(), 2);
  std::for_each(localStaticNbrs.begin(), localStaticNbrs.end(), [](auto ts) {
    EXPECT_GT(ts, 0);
  });
  // Add node
  config.dsfNodes()->insert({5, makeDsfNodeCfg(5)});
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());

  ASSERT_NE(nullptr, stateV2);
  EXPECT_EQ(stateV2->getDsfNodes()->numNodes(), 3);
  EXPECT_EQ(
      stateV2->getRemoteSystemPorts()->numNodes(),
      stateV1->getRemoteSystemPorts()->numNodes() + 1);
  EXPECT_EQ(
      stateV2->getRemoteInterfaces()->numNodes(),
      stateV1->getRemoteInterfaces()->numNodes() + 1);
  // Expect valid resolved timestamp
  auto initialTsRemote = getStaticNbrTimestamps(stateV1->getRemoteInterfaces());
  std::for_each(initialTsRemote.begin(), initialTsRemote.end(), [](auto ts) {
    EXPECT_GT(ts, 0);
  });
  // No change in local static nbrs
  EXPECT_EQ(localStaticNbrs, getStaticNbrTimestamps(stateV1->getInterfaces()));

  // Config update with no change in dsf nodes
  *config.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  EXPECT_EQ(stateV3->getDsfNodes()->numNodes(), 3);
  EXPECT_EQ(
      stateV3->getRemoteSystemPorts()->numNodes(),
      stateV2->getRemoteSystemPorts()->numNodes());
  EXPECT_EQ(
      stateV3->getRemoteInterfaces()->numNodes(),
      stateV2->getRemoteInterfaces()->numNodes());
  // No change in resolved timestamp
  EXPECT_EQ(localStaticNbrs, getStaticNbrTimestamps(stateV1->getInterfaces()));
  EXPECT_EQ(
      initialTsRemote, getStaticNbrTimestamps(stateV1->getRemoteInterfaces()));
  // Add new FN node
  config.dsfNodes()->insert(
      {6, makeDsfNodeCfg(6, cfg::DsfNodeType::FABRIC_NODE)});
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());

  ASSERT_NE(nullptr, stateV4);
  EXPECT_EQ(stateV4->getDsfNodes()->numNodes(), 4);
  EXPECT_EQ(
      stateV4->getRemoteSystemPorts()->numNodes(),
      stateV3->getRemoteSystemPorts()->numNodes());
  EXPECT_EQ(
      stateV4->getRemoteInterfaces()->numNodes(),
      stateV3->getRemoteInterfaces()->numNodes());
  // No change in resolved timestamp
  EXPECT_EQ(localStaticNbrs, getStaticNbrTimestamps(stateV1->getInterfaces()));
  EXPECT_EQ(
      initialTsRemote, getStaticNbrTimestamps(stateV1->getRemoteInterfaces()));
  // Sleep 2 sec to get a new resolved timestamp
  // @lint-ignore CLANGTIDY
  sleep(2);

  // Update IN node 5
  config.dsfNodes()->erase(5);
  auto updatedDsfNode = makeDsfNodeCfg(5);
  updatedDsfNode.name() = "newName";
  config.dsfNodes()->insert({5, updatedDsfNode});
  auto stateV5 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV5);
  EXPECT_EQ(stateV5->getDsfNodes()->numNodes(), 4);
  EXPECT_EQ(
      stateV5->getRemoteSystemPorts()->numNodes(),
      stateV2->getRemoteSystemPorts()->numNodes());
  EXPECT_EQ(
      stateV5->getRemoteInterfaces()->numNodes(),
      stateV2->getRemoteInterfaces()->numNodes());
  auto newTsRemote = getStaticNbrTimestamps(stateV1->getRemoteInterfaces());
  EXPECT_EQ(initialTsRemote.size(), newTsRemote.size());
  // Node updated - resolved timestamp should be updated
  for (auto i = 0; i < initialTsRemote.size(); ++i) {
    EXPECT_LT(initialTsRemote[i], newTsRemote[i]);
  }
  // No change in local static nbrs
  EXPECT_EQ(localStaticNbrs, getStaticNbrTimestamps(stateV1->getInterfaces()));
  // Erase node
  config.dsfNodes()->erase(5);
  auto stateV6 = publishAndApplyConfig(stateV5, &config, platform.get());
  ASSERT_NE(nullptr, stateV6);
  EXPECT_EQ(stateV6->getDsfNodes()->numNodes(), 3);
  EXPECT_EQ(
      stateV6->getRemoteSystemPorts()->numNodes(),
      stateV5->getRemoteSystemPorts()->numNodes() - 1);
  EXPECT_EQ(
      stateV6->getRemoteInterfaces()->numNodes(),
      stateV5->getRemoteInterfaces()->numNodes() - 1);
  // No remote static neighbor again
  EXPECT_EQ(getStaticNbrTimestamps(stateV1->getRemoteInterfaces()).size(), 0);
  // No change in local static nbrs
  EXPECT_EQ(localStaticNbrs, getStaticNbrTimestamps(stateV1->getInterfaces()));
}

TEST(DsfNode, dsfNodeUpdateLocalDsfNodeConfig) {
  auto platform = createMockPlatform(cfg::SwitchType::VOQ, kVoqSwitchIdBegin);
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(stateV1->getDsfNodes()->numNodes(), 2);
  EXPECT_GT(config.dsfNodes()[kVoqSwitchIdBegin].loopbackIps()->size(), 0);
  config.dsfNodes()[kVoqSwitchIdBegin].loopbackIps()->clear();
  EXPECT_EQ(config.dsfNodes()[kVoqSwitchIdBegin].loopbackIps()->size(), 0);
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  EXPECT_EQ(stateV1->getDsfNodes()->numNodes(), 2);
}

TEST(DSFNode, loopackIpsSorted) {
  auto dsfNode = makeDsfNode();
  std::vector<std::string> loopbacks{"100.1.1.0/24", "201::1/64", "300::4/64"};
  dsfNode->setLoopbackIps(loopbacks);
  std::set<folly::CIDRNetwork> loopbacksSorted;
  std::for_each(
      loopbacks.begin(),
      loopbacks.end(),
      [&loopbacksSorted](const auto& loopback) {
        loopbacksSorted.insert(
            folly::IPAddress::createNetwork(loopback, -1, false));
      });
  EXPECT_EQ(loopbacksSorted, dsfNode->getLoopbackIpsSorted());
}
