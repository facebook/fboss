/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <cstdint>
#include <optional>

namespace facebook::fboss {
class StateDelta;

class EcmpResourceManagerConfig {
 public:
  EcmpResourceManagerConfig(
      uint32_t maxHwEcmpGroups,
      std::optional<cfg::SwitchingMode> backupEcmpGroupType);
  EcmpResourceManagerConfig(
      uint32_t maxHwEcmpGroups,
      std::optional<uint32_t> maxHwEcmpMembers,
      std::optional<uint32_t> maxEcmpWidth,
      int compressionPenaltyThresholdPct);
  explicit EcmpResourceManagerConfig(uint32_t maxHwEcmpGroups)
      : EcmpResourceManagerConfig(maxHwEcmpGroups, std::nullopt) {}

  bool ecmpLimitReached(uint32_t primaryEcmpGroups, uint32_t ecmpGroupMembers)
      const;

  std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode() const {
    return backupEcmpGroupType_;
  }
  int32_t getEcmpCompressionThresholdPct() const {
    return compressionPenaltyThresholdPct_;
  }
  uint32_t getMaxPrimaryEcmpGroups() const {
    return maxHwEcmpGroups_;
  }
  void handleSwitchSettingsDelta(const StateDelta& delta);
  void handleFlowletSwitchConfigDelta(const StateDelta& delta);
  void validateCfgUpdate(
      uint32_t compressionPenaltyThresholdPct,
      const std::optional<cfg::SwitchingMode>& backupEcmpGroupType) const;

 private:
  static uint32_t computeMaxHwEcmpGroups(uint32_t maxHwEcmpGroups);
  static std::optional<uint32_t> computeMaxHwEcmpMembers(
      std::optional<uint32_t> maxHwEcmpMembers,
      std::optional<uint32_t> maxEcmpWidth);

  const uint32_t maxHwEcmpGroups_;
  const std::optional<uint32_t> maxHwEcmpMembers_;
  uint32_t compressionPenaltyThresholdPct_{0};
  std::optional<cfg::SwitchingMode> backupEcmpGroupType_;
};

} // namespace facebook::fboss
