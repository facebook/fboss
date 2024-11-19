/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss::utility {

namespace {

int getDsfInterfaceNodeCount() {
  return getMaxRdsw() + getMaxEdsw();
}

} // namespace
std::optional<std::map<int64_t, cfg::DsfNode>> addRemoteIntfNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    std::optional<int> numRemoteNodes) {
  CHECK(!curDsfNodes.empty());
  auto dsfNodes = curDsfNodes;
  const auto& firstDsfNode = dsfNodes.begin()->second;
  CHECK(!firstDsfNode.systemPortRanges()->systemPortRanges()->empty());
  CHECK(firstDsfNode.nodeMac().has_value());
  CHECK(firstDsfNode.localSystemPortOffset().has_value());
  CHECK(firstDsfNode.globalSystemPortOffset().has_value());
  CHECK(firstDsfNode.inbandPortId().has_value());
  CHECK_EQ(*firstDsfNode.switchId(), 0);
  CHECK(*firstDsfNode.type() == cfg::DsfNodeType::INTERFACE_NODE);
  cfg::SwitchInfo switchInfo;
  switchInfo.asicType() = *firstDsfNode.asicType();
  switchInfo.switchType() = cfg::SwitchType::VOQ;
  switchInfo.switchIndex() = 0;
  switchInfo.switchMac() = *firstDsfNode.nodeMac();
  switchInfo.systemPortRanges() = *firstDsfNode.systemPortRanges();
  switchInfo.localSystemPortOffset() = *firstDsfNode.localSystemPortOffset();
  switchInfo.globalSystemPortOffset() = *firstDsfNode.globalSystemPortOffset();
  switchInfo.inbandPortId() = *firstDsfNode.inbandPortId();

  auto asic =
      HwAsic::makeAsic(*firstDsfNode.switchId(), switchInfo, std::nullopt);
  int numCores = asic->getNumCores();
  CHECK(
      !numRemoteNodes.has_value() ||
      numRemoteNodes.value() < getDsfInterfaceNodeCount());
  int totalNodes = numRemoteNodes.has_value()
      ? numRemoteNodes.value() + curDsfNodes.size()
      : getDsfInterfaceNodeCount();
  auto lastDsfNode = dsfNodes.rbegin()->second;
  int remoteNodeStart = *lastDsfNode.switchId() + numCores;
  auto firstDsfNodeSysPortRanges =
      *firstDsfNode.systemPortRanges()->systemPortRanges();

  for (int remoteSwitchId = remoteNodeStart;
       remoteSwitchId < totalNodes * numCores;
       remoteSwitchId += numCores) {
    auto remoteDsfNodeCfg = dsfNodeConfig(
        *asic, SwitchID(remoteSwitchId), *firstDsfNode.platformType());
    dsfNodes.insert({remoteSwitchId, remoteDsfNodeCfg});
  }
  return dsfNodes;
}

std::pair<int, cfg::DsfNode> getRemoteFabricNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    int fabricLevel,
    int clusterId,
    cfg::AsicType asicType,
    PlatformType platformType) {
  CHECK(!curDsfNodes.empty());
  auto dsfNodes = curDsfNodes;

  auto getRemoteSwitchID = [](cfg::DsfNode& dsfNode) {
    const auto& lastNodeHwAsic = getHwAsicForAsicType(*dsfNode.asicType());
    return *dsfNode.switchId() + lastNodeHwAsic.getNumCores();
  };

  int remoteSwitchId = getRemoteSwitchID(dsfNodes.rbegin()->second);
  auto nodeName = folly::to<std::string>("fabNode", remoteSwitchId);
  cfg::DsfNode fabricNode;
  fabricNode.name() = nodeName;
  fabricNode.switchId() = remoteSwitchId;
  fabricNode.type() = cfg::DsfNodeType::FABRIC_NODE;
  fabricNode.asicType() = asicType;
  fabricNode.platformType() = platformType;
  fabricNode.clusterId() = clusterId;
  fabricNode.fabricLevel() = fabricLevel;
  return {remoteSwitchId, fabricNode};
}
} // namespace facebook::fboss::utility
