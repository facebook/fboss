// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <unordered_set>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchInfoTable {
 public:
  SwitchInfoTable(
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);
  std::unordered_set<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  std::unordered_set<SwitchID> getL3SwitchIDs() const;
  std::unordered_set<SwitchID> getSwitchIDs() const;
  std::unordered_set<uint16_t> getSwitchIndicesOfType(
      cfg::SwitchType type) const;
  std::unordered_set<uint16_t> getSwitchIndices() const;
  bool haveVoqSwitches() const;
  bool haveNpuSwitches() const;
  bool haveFabricSwitches() const;
  bool haveL3Switches() const;
  bool vlansSupported() const;
  cfg::SwitchType l3SwitchType() const;
  const std::map<SwitchID, cfg::SwitchInfo>& getSwitchIdToSwitchInfo() const {
    return switchIdToSwitchInfo_;
  }
  int16_t getSwitchIndexFromSwitchId(SwitchID switchId) const;
  cfg::SwitchInfo getSwitchInfo(SwitchID switchId) const;

 private:
  std::map<SwitchID, cfg::SwitchInfo> switchIdToSwitchInfo_;
};

} // namespace facebook::fboss
