// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwSwitch;

class SwitchInfoTable {
 public:
  explicit SwitchInfoTable(
      SwSwitch* sw,
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);
  std::vector<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  std::vector<SwitchID> getSwitchIDs() const;
  bool vlansSupported() const;
  cfg::SwitchType l3SwitchType() const;

 private:
  SwSwitch* sw_;
  std::map<SwitchID, cfg::SwitchInfo> switchIdToSwitchInfo_;
};

} // namespace facebook::fboss
