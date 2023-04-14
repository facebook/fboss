// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchInfoTable {
 public:
  explicit SwitchInfoTable(
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);
  std::vector<SwitchID> getSwitchIdsOfType(cfg::SwitchType type);

 private:
  std::map<SwitchID, cfg::SwitchInfo> switchIdToSwitchInfo_;
};

} // namespace facebook::fboss
