/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/EcmpResourceManagerConfig.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

EcmpResourceManagerConfig::EcmpResourceManagerConfig(
    uint32_t maxHwEcmpGroups,
    std::optional<cfg::SwitchingMode> backupEcmpGroupType)
    : maxHwEcmpGroups_(computeMaxHwEcmpGroups(maxHwEcmpGroups)),
      backupEcmpGroupType_(backupEcmpGroupType) {}

EcmpResourceManagerConfig::EcmpResourceManagerConfig(
    uint32_t maxHwEcmpGroups,
    std::optional<uint32_t> maxHwEcmpMembers,
    std::optional<uint32_t> maxEcmpWidth,
    int compressionPenaltyThresholdPct)
    : maxHwEcmpGroups_(computeMaxHwEcmpGroups(maxHwEcmpGroups)),
      maxHwEcmpMembers_(
          computeMaxHwEcmpMembers(maxHwEcmpMembers, maxEcmpWidth)),
      compressionPenaltyThresholdPct_(compressionPenaltyThresholdPct) {
  CHECK_LE(compressionPenaltyThresholdPct_, 100);
}

bool EcmpResourceManagerConfig::ecmpLimitReached(
    uint32_t primaryEcmpGroups,
    uint32_t ecmpGroupMembers) const {
  DCHECK_LE(primaryEcmpGroups, maxHwEcmpGroups_);
  DCHECK(!maxHwEcmpMembers_ || ecmpGroupMembers <= *maxHwEcmpMembers_);
  return primaryEcmpGroups >= maxHwEcmpGroups_ ||
      (maxHwEcmpMembers_.has_value() && ecmpGroupMembers >= *maxHwEcmpMembers_);
}

uint32_t EcmpResourceManagerConfig::computeMaxHwEcmpGroups(
    uint32_t maxHwEcmpGroups) {
  CHECK_GT(
      maxHwEcmpGroups, FLAGS_ecmp_resource_manager_make_before_break_buffer);
  // We keep a buffer of 2 for transient increment in ECMP groups when
  // pushing updates down to HW
  return maxHwEcmpGroups - FLAGS_ecmp_resource_manager_make_before_break_buffer;
}

std::optional<uint32_t> EcmpResourceManagerConfig::computeMaxHwEcmpMembers(
    std::optional<uint32_t> maxHwEcmpMembers,
    std::optional<uint32_t> maxEcmpWidth) {
  CHECK_EQ(maxHwEcmpMembers.has_value(), maxEcmpWidth.has_value());
  if (!maxHwEcmpMembers.has_value()) {
    return std::nullopt;
  }
  int maxMembers = *maxHwEcmpMembers -
      FLAGS_ecmp_resource_manager_make_before_break_buffer * *maxEcmpWidth;
  CHECK_GT(maxMembers, 0);
  return maxMembers;
}

void EcmpResourceManagerConfig::validateCfgUpdate(
    uint32_t compressionPenaltyThresholdPct,
    const std::optional<cfg::SwitchingMode>& backupEcmpGroupType) const {
  if (compressionPenaltyThresholdPct && backupEcmpGroupType.has_value()) {
    throw FbossError(
        "Setting both compression threshold pct and backup ecmp group type is not supported");
  }
}

void EcmpResourceManagerConfig::handleSwitchSettingsDelta(
    const StateDelta& delta) {
  if (DeltaFunctions::isEmpty(delta.getSwitchSettingsDelta())) {
    return;
  }
  std::optional<int32_t> newEcmpCompressionThresholdPct;
  std::vector<std::optional<int32_t>> newEcmpCompressionThresholdPcts;
  for (const auto& [_, switchSettings] :
       std::as_const(*delta.newState()->getSwitchSettings())) {
    newEcmpCompressionThresholdPcts.emplace_back(
        switchSettings->getEcmpCompressionThresholdPct());
  }
  if (newEcmpCompressionThresholdPcts.size()) {
    newEcmpCompressionThresholdPct = *newEcmpCompressionThresholdPcts.begin();
    std::for_each(
        newEcmpCompressionThresholdPcts.begin(),
        newEcmpCompressionThresholdPcts.end(),
        [&newEcmpCompressionThresholdPct](
            const auto& ecmpCompressionThresholdPct) {
          if (ecmpCompressionThresholdPct != newEcmpCompressionThresholdPct) {
            throw FbossError(
                "All switches must have same ecmp compression threshold value");
          }
        });
  }
  if (newEcmpCompressionThresholdPct.value_or(0) ==
      compressionPenaltyThresholdPct_) {
    return;
  }
  /*
   * compressionPenaltyThresholdPct_ was already non-zero. Changing
   * is not allowed. Otherwise we may now get some routes that are out
   * of compliance
   */
  if (compressionPenaltyThresholdPct_ != 0) {
    throw FbossError(
        "Changing compression penalty threshold on the fly is not supported");
  }
  validateCfgUpdate(
      newEcmpCompressionThresholdPct.value_or(0), backupEcmpGroupType_);
  compressionPenaltyThresholdPct_ = newEcmpCompressionThresholdPct.value_or(0);
}

void EcmpResourceManagerConfig::handleFlowletSwitchConfigDelta(
    const StateDelta& delta) {
  if (delta.getFlowletSwitchingConfigDelta().getOld() ==
      delta.getFlowletSwitchingConfigDelta().getNew()) {
    return;
  }
  std::optional<cfg::SwitchingMode> newMode;
  if (delta.newState()->getFlowletSwitchingConfig()) {
    newMode =
        delta.newState()->getFlowletSwitchingConfig()->getBackupSwitchingMode();
  }
  if (backupEcmpGroupType_.has_value() && !newMode.has_value()) {
    throw FbossError(
        "Cannot change backup ecmp switching mode from non-null to null");
  }
  if (backupEcmpGroupType_ == newMode) {
    return;
  }

  XLOG(DBG2) << "Updating backup switching mode from: "
             << (backupEcmpGroupType_.has_value()
                     ? apache::thrift::util::enumNameSafe(*backupEcmpGroupType_)
                     : "None")
             << " to: " << apache::thrift::util::enumNameSafe(*newMode);

  validateCfgUpdate(getEcmpCompressionThresholdPct(), newMode);
  backupEcmpGroupType_ = newMode;
}

} // namespace facebook::fboss
