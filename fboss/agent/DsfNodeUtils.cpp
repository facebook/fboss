/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/DsfNodeUtils.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/VoqConstants.h"

namespace facebook::fboss::utility {

namespace {

uint32_t getDsfMaxSwitchId(bool isDualStageTopology) {
  if (isDualStageTopology) {
    // TODO: look at 2-stage configs, find max switch-id and use that
    // to compute the value here.
    return kDualStageMaxGlobalSwitchId;
  } else {
    // TODO: Programatically calculate the max switch-id and
    // assert that we are are within this limit
    return kSingleStageMaxGlobalSwitchId;
  }
}

} // namespace

bool isDualStage(const AgentConfig& cfg) {
  return isDualStage(cfg.thrift);
}
bool isDualStage(const cfg::AgentConfig& cfg) {
  return isDualStage(*cfg.sw());
}
bool isDualStage(const cfg::SwitchConfig& cfg) {
  for (auto const& [_, dsfNode] : *cfg.dsfNodes()) {
    if (dsfNode.fabricLevel().has_value() && dsfNode.fabricLevel() == 2) {
      return true;
    }
  }
  return false;
}

int64_t maxDsfSwitchId(const AgentConfig& cfg) {
  return maxDsfSwitchId(cfg.thrift);
}
int64_t maxDsfSwitchId(const cfg::AgentConfig& cfg) {
  return maxDsfSwitchId(*cfg.sw());
}

int64_t maxDsfSwitchId(const cfg::SwitchConfig& cfg) {
  std::optional<int64_t> maxSwitchId;
  for (auto const& [_, dsfNode] : *cfg.dsfNodes()) {
    maxSwitchId = std::max(*dsfNode.switchId(), maxSwitchId.value_or(0));
  }
  if (!maxSwitchId.has_value()) {
    throw FbossError("No DSF Node switch id found");
  }
  return maxSwitchId.value();
}

uint32_t getDsfVoqSwitchMaxSwitchId() {
  return getDsfMaxSwitchId(isDualStage3Q2QMode());
}

uint32_t getDsfFabricSwitchMaxSwitchId(
    const AgentConfig& agentConfig,
    HwAsic::FabricNodeRole fabricNodeRole) {
  // Identify if the fabric switch is from a dual stage DSF cluster
  // and pass it in as param to find the max switch ID.
  CHECK_EQ(
      isDualStage(agentConfig),
      (fabricNodeRole == HwAsic::FabricNodeRole::DUAL_STAGE_L1) ||
          (fabricNodeRole == HwAsic::FabricNodeRole::DUAL_STAGE_L2));
  // Given equality is confirmed above, simplifying the param passed
  return getDsfMaxSwitchId(isDualStage(agentConfig));
}

} // namespace facebook::fboss::utility
