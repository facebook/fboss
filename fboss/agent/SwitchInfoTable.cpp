// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchInfoTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
SwitchInfoTable::SwitchInfoTable(
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo) {
  std::optional<cfg::SwitchType> switchType;
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo) {
    if (switchType.has_value() &&
        switchType.value() != switchIdAndSwitchInfo.second.switchType()) {
      throw FbossError(
          "Mix of different switch types in SwitchInfoTable is not supported. SwitchTypes ",
          switchType.value(),
          *switchIdAndSwitchInfo.second.switchType());
    } else {
      switchType = *switchIdAndSwitchInfo.second.switchType();
    }
    switchIdToSwitchInfo_.emplace(
        SwitchID(switchIdAndSwitchInfo.first), switchIdAndSwitchInfo.second);
  }
}

std::unordered_set<SwitchID> SwitchInfoTable::getSwitchIdsOfType(
    cfg::SwitchType type) const {
  std::unordered_set<SwitchID> switchIds;
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo_) {
    if (switchIdAndSwitchInfo.second.switchType() == type) {
      switchIds.insert(switchIdAndSwitchInfo.first);
    }
  }
  return switchIds;
}

std::unordered_set<SwitchID> SwitchInfoTable::getL3SwitchIDs() const {
  return haveL3Switches() ? getSwitchIdsOfType(l3SwitchType())
                          : std::unordered_set<SwitchID>();
}

std::unordered_set<SwitchID> SwitchInfoTable::getSwitchIDs() const {
  CHECK_NE(switchIdToSwitchInfo_.size(), 0);
  std::unordered_set<SwitchID> switchIds;
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo_) {
    switchIds.insert(switchIdAndSwitchInfo.first);
  }
  return switchIds;
}

std::unordered_set<uint16_t> SwitchInfoTable::getSwitchIndicesOfType(
    cfg::SwitchType type) const {
  std::unordered_set<uint16_t> switchIdxs;
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo_) {
    if (switchIdAndSwitchInfo.second.switchType() == type) {
      switchIdxs.insert(*switchIdAndSwitchInfo.second.switchIndex());
    }
  }
  return switchIdxs;
}

std::unordered_set<uint16_t> SwitchInfoTable::getSwitchIndices() const {
  std::unordered_set<uint16_t> switchIdxs;
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo_) {
    switchIdxs.insert(*switchIdAndSwitchInfo.second.switchIndex());
  }
  return switchIdxs;
}
bool SwitchInfoTable::vlansSupported() const {
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo_) {
    if (switchIdAndSwitchInfo.second.switchType() == cfg::SwitchType::FABRIC ||
        switchIdAndSwitchInfo.second.switchType() == cfg::SwitchType::VOQ) {
      return false;
    }
  }
  return true;
}

cfg::SwitchType SwitchInfoTable::l3SwitchType() const {
  /*
   * We don't allow mixing multiple l3 switch types (since
   * these have different programming models). So look for
   * the first l3 switch type and return that
   */
  if (getSwitchIdsOfType(cfg::SwitchType::NPU).size()) {
    return cfg::SwitchType::NPU;
  }
  if (getSwitchIdsOfType(cfg::SwitchType::VOQ).size()) {
    return cfg::SwitchType::VOQ;
  }
  throw FbossError("No L3 NPUs found");
}

bool SwitchInfoTable::haveVoqSwitches() const {
  return getSwitchIdsOfType(cfg::SwitchType::VOQ).size() > 0;
}

bool SwitchInfoTable::haveNpuSwitches() const {
  return getSwitchIdsOfType(cfg::SwitchType::NPU).size() > 0;
}

bool SwitchInfoTable::haveFabricSwitches() const {
  return getSwitchIdsOfType(cfg::SwitchType::FABRIC).size() > 0;
}

bool SwitchInfoTable::haveL3Switches() const {
  return haveVoqSwitches() || haveNpuSwitches();
}

int16_t SwitchInfoTable::getSwitchIndexFromSwitchId(SwitchID switchId) const {
  if (switchIdToSwitchInfo_.find(switchId) == switchIdToSwitchInfo_.end()) {
    throw FbossError("Invalid switch ID: ", switchId);
  }
  return switchIdToSwitchInfo_.at(switchId).switchIndex().value();
}

cfg::SwitchInfo SwitchInfoTable::getSwitchInfo(SwitchID switchId) const {
  auto swItr = switchIdToSwitchInfo_.find(switchId);
  if (swItr == switchIdToSwitchInfo_.end()) {
    throw FbossError("Could not find switchInfo for switch ID: ", switchId);
  }
  return swItr->second;
}
} // namespace facebook::fboss
