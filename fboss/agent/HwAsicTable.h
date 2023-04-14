// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwSwitch;

class HwAsicTable {
 public:
  HwAsicTable(
      SwSwitch* sw,
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);
  const HwAsic* getHwAsicIf(SwitchID switchID) const;

 private:
  SwSwitch* sw_;
  std::map<SwitchID, std::unique_ptr<HwAsic>> hwAsics_;
};

} // namespace facebook::fboss
