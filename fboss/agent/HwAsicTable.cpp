// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/HwAsicTable.h"
#include <optional>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMappingUtils.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {
HwAsicTable::HwAsicTable(
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo) {
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo) {
    // TODO - pass proper mac to asic create
    folly::MacAddress mac;
    hwAsics_.emplace(
        SwitchID(switchIdAndSwitchInfo.first),
        HwAsic::makeAsic(
            *switchIdAndSwitchInfo.second.asicType(),
            *switchIdAndSwitchInfo.second.switchType(),
            switchIdAndSwitchInfo.first,
            std::nullopt,
            mac));
  }
}

const HwAsic* HwAsicTable::getHwAsicIf(SwitchID switchID) const {
  auto iter = hwAsics_.find(switchID);
  if (iter != hwAsics_.end()) {
    return iter->second.get();
  }
  return nullptr;
}

bool HwAsicTable::isFeatureSupported(SwitchID switchId, HwAsic::Feature feature)
    const {
  auto asic = getHwAsicIf(switchId);
  CHECK(asic);
  return asic->isSupported(feature);
}

bool HwAsicTable::isFeatureSupportedOnAnyAsic(HwAsic::Feature feature) const {
  for (const auto& entry : hwAsics_) {
    if (isFeatureSupported(entry.first, feature)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> HwAsicTable::asicNames() const {
  std::vector<std::string> asicNames;
  for (const auto& entry : hwAsics_) {
    asicNames.emplace_back(entry.second->getAsicTypeStr());
  }
  return asicNames;
}

std::set<cfg::StreamType> HwAsicTable::getCpuPortQueueStreamTypes() const {
  std::set<cfg::StreamType> streamTypes;
  for (const auto& switchId : getL3SwitchIds()) {
    auto asic = getHwAsicIf(switchId);
    if (asic->isSupported(HwAsic::Feature::CPU_PORT)) {
      if (streamTypes.empty()) {
        streamTypes = asic->getQueueStreamTypes(cfg::PortType::CPU_PORT);
      } else if (
          streamTypes != asic->getQueueStreamTypes(cfg::PortType::CPU_PORT)) {
        throw FbossError("Cannot support switches with different stream types");
      }
    }
  }
  if (streamTypes.empty()) {
    throw FbossError("No Switches with CPU port support present");
  }
  return streamTypes;
}

std::unordered_set<SwitchID> HwAsicTable::getSwitchIdsOfType(
    cfg::SwitchType type) const {
  std::unordered_set<SwitchID> switchIds;
  for (const auto& entry : hwAsics_) {
    if (entry.second->getSwitchType() == type) {
      switchIds.insert(entry.first);
    }
  }
  return switchIds;
}

std::unordered_set<SwitchID> HwAsicTable::getL3SwitchIds() const {
  std::unordered_set<SwitchID> switchIds;
  for (const auto switchType : {cfg::SwitchType::NPU, cfg::SwitchType::VOQ}) {
    const auto typeSwitchIds = getSwitchIdsOfType(switchType);
    switchIds.insert(typeSwitchIds.begin(), typeSwitchIds.end());
  }
  return switchIds;
}

std::set<cfg::StreamType> HwAsicTable::getQueueStreamTypes(
    SwitchID switchId,
    cfg::PortType portType) const {
  auto asic = getHwAsicIf(switchId);
  if (!asic) {
    throw FbossError("Invalid switch ID for getQueueStreamTypes ", switchId);
  }
  return asic->getQueueStreamTypes(portType);
}

} // namespace facebook::fboss
