// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <unordered_set>
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwAsicTable {
 public:
  explicit HwAsicTable(
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);
  const HwAsic* getHwAsicIf(SwitchID switchID) const;
  HwAsic* getHwAsicIf(SwitchID switchID);
  const HwAsic* getHwAsic(SwitchID switchID) const;
  bool isFeatureSupported(SwitchID switchId, HwAsic::Feature feature) const;
  bool isFeatureSupportedOnAnyAsic(HwAsic::Feature feature) const;
  std::vector<std::string> asicNames() const;
  std::set<cfg::StreamType> getCpuPortQueueStreamTypes() const;
  std::set<cfg::StreamType> getQueueStreamTypes(
      SwitchID switchId,
      cfg::PortType portType) const;
  std::unordered_set<SwitchID> getSwitchIDs() const;

 private:
  HwAsic* getHwAsicIfImpl(SwitchID switchID) const;
  std::unordered_set<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  std::unordered_set<SwitchID> getL3SwitchIds() const;
  std::map<SwitchID, std::unique_ptr<HwAsic>> hwAsics_;
};

} // namespace facebook::fboss
