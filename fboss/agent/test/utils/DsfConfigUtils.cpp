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
constexpr auto kNumRdsw = 128;
constexpr auto kNumEdsw = 16;
constexpr auto kNumRdswSysPort = 44;
constexpr auto kNumEdswSysPort = 26;
constexpr auto kJ2NumSysPort = 20;

int getPerNodeSysPorts(const HwAsic& asic, int remoteSwitchId) {
  if (asic.getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
    return kJ2NumSysPort;
  }
  if (remoteSwitchId < kNumRdsw * asic.getNumCores()) {
    return kNumRdswSysPort;
  }
  return kNumEdswSysPort;
}
} // namespace

int getDsfNodeCount(const HwAsic& asic) {
  return asic.getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2
      ? kNumRdsw
      : kNumRdsw + kNumEdsw;
}

std::optional<std::map<int64_t, cfg::DsfNode>> addRemoteIntfNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    std::optional<int> numRemoteNodes) {
  CHECK(!curDsfNodes.empty());
  auto dsfNodes = curDsfNodes;
  const auto& firstDsfNode = dsfNodes.begin()->second;
  CHECK(firstDsfNode.systemPortRange().has_value());
  CHECK(firstDsfNode.nodeMac().has_value());
  auto asic = HwAsic::makeAsic(
      *firstDsfNode.asicType(),
      cfg::SwitchType::VOQ,
      *firstDsfNode.switchId(),
      0,
      *firstDsfNode.systemPortRange(),
      folly::MacAddress(*firstDsfNode.nodeMac()),
      std::nullopt);
  int numCores = asic->getNumCores();
  CHECK(
      !numRemoteNodes.has_value() ||
      numRemoteNodes.value() < getDsfNodeCount(*asic));
  int totalNodes = numRemoteNodes.has_value()
      ? numRemoteNodes.value() + curDsfNodes.size()
      : getDsfNodeCount(*asic);
  int remoteNodeStart = dsfNodes.rbegin()->first + numCores;
  int systemPortMin =
      getPerNodeSysPorts(*asic, dsfNodes.begin()->first) * numCores;
  for (int remoteSwitchId = remoteNodeStart;
       remoteSwitchId < totalNodes * numCores;
       remoteSwitchId += numCores) {
    cfg::Range64 systemPortRange;
    systemPortRange.minimum() = systemPortMin;
    systemPortRange.maximum() =
        systemPortMin + getPerNodeSysPorts(*asic, remoteSwitchId) - 1;
    auto remoteDsfNodeCfg = dsfNodeConfig(
        *asic,
        SwitchID(remoteSwitchId),
        systemPortMin,
        *systemPortRange.maximum(),
        *firstDsfNode.platformType());
    dsfNodes.insert({remoteSwitchId, remoteDsfNodeCfg});
    systemPortMin = *systemPortRange.maximum() + 1;
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
