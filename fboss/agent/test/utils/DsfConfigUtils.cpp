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
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss::utility {

namespace {
constexpr auto kNumRdsw = 128;
constexpr auto kNumEdsw = 16;
constexpr auto kNumRdswSysPort = 44;
constexpr auto kNumEdswSysPort = 26;
constexpr auto kJ2NumSysPort = 20;

int getPerNodeSysPorts(const HwAsic* asic, int remoteSwitchId) {
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
    return kJ2NumSysPort;
  }
  if (remoteSwitchId < kNumRdsw * asic->getNumCores()) {
    return kNumRdswSysPort;
  }
  return kNumEdswSysPort;
}
} // namespace

int getDsfNodeCount(const HwAsic* asic) {
  return asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2
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
  folly::MacAddress mac(*firstDsfNode.nodeMac());
  auto asic = HwAsic::makeAsic(
      *firstDsfNode.asicType(),
      cfg::SwitchType::VOQ,
      *firstDsfNode.switchId(),
      0,
      *firstDsfNode.systemPortRange(),
      mac,
      std::nullopt);
  int numCores = asic->getNumCores();
  CHECK(
      !numRemoteNodes.has_value() ||
      numRemoteNodes.value() < getDsfNodeCount(asic.get()));
  int totalNodes = numRemoteNodes.has_value() ? numRemoteNodes.value() + 1
                                              : getDsfNodeCount(asic.get());
  int systemPortMin = getPerNodeSysPorts(asic.get(), 1);
  for (int remoteSwitchId = numCores; remoteSwitchId < totalNodes * numCores;
       remoteSwitchId += numCores) {
    cfg::Range64 systemPortRange;
    systemPortRange.minimum() = systemPortMin;
    systemPortRange.maximum() =
        systemPortMin + getPerNodeSysPorts(asic.get(), remoteSwitchId) - 1;
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
} // namespace facebook::fboss::utility
