// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/DataStore.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_snapshot_types.h"

namespace facebook::fboss::platform::platform_manager {

class PlatformSnapshotBuilder {
 public:
  PlatformSnapshotBuilder(
      const PlatformConfig& config,
      const DataStore& dataStore);

  PlatformSnapshot build() const;

 private:
  PMUnit buildPMUnit(const std::string& slotPath, const std::string& pmUnitName)
      const;

  Slot buildSlot(
      const std::string& parentSlotPath,
      const std::string& slotName,
      const SlotConfig& slotConfig) const;

  const PlatformConfig& platformConfig_;
  const DataStore& dataStore_;
};

} // namespace facebook::fboss::platform::platform_manager
