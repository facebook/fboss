// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

#include <unordered_set>

namespace facebook::fboss {

class SwSwitch;

class SwitchInfoTable {
 public:
  SwitchInfoTable(
      SwSwitch* sw,
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);
  std::unordered_set<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  std::unordered_set<SwitchID> getSwitchIDs() const;
  bool haveVoqSwitches() const;
  bool haveNpuSwitches() const;
  bool haveL3Switches() const;
  bool vlansSupported() const;
  cfg::SwitchType l3SwitchType() const;

 private:
  SwSwitch* sw_;
  std::map<SwitchID, cfg::SwitchInfo> switchIdToSwitchInfo_;
};

} // namespace facebook::fboss
