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

#include <cmath>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

EcmpResourceManagerConfig::EcmpResourceManagerConfig(
    uint32_t maxHwEcmpGroups,
    std::optional<cfg::SwitchingMode> backupEcmpGroupType,
    std::optional<uint32_t> maxVirtualEcmpGroups,
    std::optional<int32_t> minWidthForVirtualGroup,
    std::optional<int32_t> maxVirtualGroupWidth,
    std::optional<int32_t> maxEcmpWidth)
    : maxHwEcmpGroups_(computeMaxHwEcmpGroups(
          maxHwEcmpGroups,
          maxVirtualEcmpGroups,
          maxVirtualGroupWidth,
          maxEcmpWidth)),
      maxVirtualEcmpGroups_(computeMaxVirtualEcmpGroups(maxVirtualEcmpGroups)),
      minWidthForVirtualGroup_(minWidthForVirtualGroup),
      maxVirtualGroupWidth_(maxVirtualGroupWidth),
      maxEcmpWidth_(maxEcmpWidth),
      backupEcmpGroupType_(backupEcmpGroupType) {
  // Both bounds must be set together: without minWidth no groups qualify
  // as virtual; without the cap, reclaim headroom is undefined.
  if (maxVirtualEcmpGroups.has_value() != minWidthForVirtualGroup.has_value()) {
    throw FbossError(
        "maxVirtualEcmpGroups and minWidthForVirtualGroup must both be set or both be null");
  }
}

EcmpResourceManagerConfig::EcmpResourceManagerConfig(
    uint32_t maxHwEcmpGroups,
    std::optional<uint32_t> maxHwEcmpMembers,
    std::optional<uint32_t> maxEcmpWidth,
    int compressionPenaltyThresholdPct)
    : maxHwEcmpGroups_(computeMaxHwEcmpGroups(
          maxHwEcmpGroups,
          std::nullopt,
          std::nullopt,
          std::nullopt)),
      maxHwEcmpMembers_(
          computeMaxHwEcmpMembers(maxHwEcmpMembers, maxEcmpWidth)),
      compressionPenaltyThresholdPct_(compressionPenaltyThresholdPct) {
  CHECK_LE(compressionPenaltyThresholdPct_, 100);
}

bool EcmpResourceManagerConfig::ecmpLimitReached(
    uint32_t primaryEcmpGroups,
    uint32_t ecmpGroupMembers,
    std::optional<uint32_t> virtualEcmpGroups) const {
  DCHECK_LE(primaryEcmpGroups, maxHwEcmpGroups_);
  DCHECK(!maxHwEcmpMembers_ || ecmpGroupMembers <= *maxHwEcmpMembers_);
  if (primaryEcmpGroups >= maxHwEcmpGroups_) {
    return true;
  }
  if (virtualEcmpGroups.has_value() && maxVirtualEcmpGroups_.has_value() &&
      *virtualEcmpGroups >= *maxVirtualEcmpGroups_) {
    return true;
  }
  if (maxHwEcmpMembers_.has_value() && ecmpGroupMembers >= *maxHwEcmpMembers_) {
    return true;
  }
  return false;
}

uint32_t EcmpResourceManagerConfig::computeMaxHwEcmpGroups(
    uint32_t maxHwEcmpGroups,
    std::optional<uint32_t> maxVirtualEcmpGroups,
    std::optional<int32_t> maxVirtualGroupWidth,
    std::optional<int32_t> maxEcmpWidth) {
  uint32_t reserved = FLAGS_ecmp_resource_manager_make_before_break_buffer;
  // Pre-reserve the collective DLB pool cost (maxVirtualGroupWidth /
  // maxEcmpWidth) so per-route accounting only needs per-group counts.
  if (maxVirtualEcmpGroups.has_value() && maxVirtualGroupWidth.has_value() &&
      maxEcmpWidth.has_value() && *maxEcmpWidth > 0) {
    reserved += static_cast<uint32_t>(*maxVirtualGroupWidth / *maxEcmpWidth);
  }
  CHECK_GT(maxHwEcmpGroups, reserved);
  return maxHwEcmpGroups - reserved;
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

std::optional<uint32_t> EcmpResourceManagerConfig::computeMaxVirtualEcmpGroups(
    std::optional<uint32_t> maxVirtualEcmpGroups) {
  if (!maxVirtualEcmpGroups.has_value()) {
    return std::nullopt;
  }
  if (*maxVirtualEcmpGroups <=
      static_cast<uint32_t>(
          FLAGS_ecmp_resource_manager_make_before_break_buffer)) {
    throw FbossError(
        "Virtual ARS max groups (",
        *maxVirtualEcmpGroups,
        ") must exceed make-before-break buffer (",
        FLAGS_ecmp_resource_manager_make_before_break_buffer,
        "). Check ars_resource_percentage config.");
  }
  return *maxVirtualEcmpGroups -
      FLAGS_ecmp_resource_manager_make_before_break_buffer;
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

  auto cfg = delta.newState()->getFlowletSwitchingConfig();

  auto newRawVirtualGroups = cfg ? cfg->getMaxArsVirtualGroups() : std::nullopt;
  auto newMinWidthForVirtualGroup =
      cfg ? cfg->getMinWidthForArsVirtualGroup() : std::nullopt;
  auto newMaxVirtualGroupWidth =
      cfg ? cfg->getMaxArsVirtualGroupWidth() : std::nullopt;
  if (maxVirtualEcmpGroups_.has_value() && !newRawVirtualGroups.has_value()) {
    throw FbossError(
        "Cannot change virtual ARS group config from non-null to null");
  }
  if (maxVirtualEcmpGroups_.has_value() && newRawVirtualGroups.has_value()) {
    auto newComputed = computeMaxVirtualEcmpGroups(
        static_cast<uint32_t>(std::floor(
            newRawVirtualGroups.value() *
            static_cast<double>(FLAGS_ars_resource_percentage) / 100.0)));
    if (newComputed != maxVirtualEcmpGroups_) {
      throw FbossError("Cannot change virtual ARS group config once bound");
    }
  }
  if (minWidthForVirtualGroup_.has_value() &&
      !newMinWidthForVirtualGroup.has_value()) {
    throw FbossError(
        "Cannot change virtual ARS min width config from non-null to null");
  }
  if (minWidthForVirtualGroup_.has_value() &&
      newMinWidthForVirtualGroup.has_value() &&
      minWidthForVirtualGroup_ != newMinWidthForVirtualGroup) {
    throw FbossError("Cannot change virtual ARS min width config once bound");
  }
  if (maxVirtualGroupWidth_.has_value() &&
      !newMaxVirtualGroupWidth.has_value()) {
    throw FbossError(
        "Cannot change virtual ARS max group width config from non-null to null");
  }
  if (maxVirtualGroupWidth_.has_value() &&
      newMaxVirtualGroupWidth.has_value() &&
      maxVirtualGroupWidth_ != newMaxVirtualGroupWidth) {
    throw FbossError(
        "Cannot change virtual ARS max group width config once bound");
  }
  if (!maxVirtualEcmpGroups_.has_value() && cfg) {
    if (newRawVirtualGroups.has_value() && newRawVirtualGroups.value() > 0) {
      std::optional<uint32_t> newMaxVirtualEcmpGroups =
          static_cast<uint32_t>(std::floor(
              newRawVirtualGroups.value() *
              static_cast<double>(FLAGS_ars_resource_percentage) / 100.0));
      if (!newMinWidthForVirtualGroup.has_value()) {
        throw FbossError(
            "maxArsVirtualGroups is set but minWidthForArsVirtualGroup is missing");
      }
      // Validate the collective DLB pool cost before mutating any state so
      // that a bad config leaves the object in a consistent state.
      std::optional<uint32_t> collectiveCost;
      if (newMaxVirtualGroupWidth.has_value() && maxEcmpWidth_.has_value() &&
          *maxEcmpWidth_ > 0) {
        collectiveCost =
            static_cast<uint32_t>(*newMaxVirtualGroupWidth / *maxEcmpWidth_);
        if (*collectiveCost >= maxHwEcmpGroups_) {
          throw FbossError(
              "Virtual ARS collective DLB pool cost ",
              *collectiveCost,
              " exceeds primary ECMP group budget ",
              maxHwEcmpGroups_);
        }
      }
      maxVirtualEcmpGroups_ =
          computeMaxVirtualEcmpGroups(newMaxVirtualEcmpGroups);
      minWidthForVirtualGroup_ = newMinWidthForVirtualGroup;
      maxVirtualGroupWidth_ = newMaxVirtualGroupWidth;
      if (collectiveCost.has_value()) {
        maxHwEcmpGroups_ -= *collectiveCost;
      }
      XLOG(DBG2) << "Bound virtual ARS config: maxVirtual="
                 << *maxVirtualEcmpGroups_
                 << ", minWidth=" << *minWidthForVirtualGroup_
                 << ", maxVirtualGroupWidth="
                 << (maxVirtualGroupWidth_ ? *maxVirtualGroupWidth_ : 0)
                 << ", adjusted maxHwEcmpGroups=" << maxHwEcmpGroups_;
    }
  }

  std::optional<cfg::SwitchingMode> newMode;
  if (cfg) {
    newMode = cfg->getBackupSwitchingMode();
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
