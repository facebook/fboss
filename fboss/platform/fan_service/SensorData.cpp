// Copyright 2021- Facebook. All rights reserved.

#include "fboss/platform/fan_service/SensorData.h"

#include "common/time/Time.h"

namespace facebook::fboss::platform::fan_service {

std::optional<SensorEntry> SensorData::getSensorEntry(
    const std::string& name) const {
  auto itr = sensorEntry_.find(name);
  if (itr == sensorEntry_.end()) {
    return std::nullopt;
  }
  return itr->second;
}

void SensorData::updateSensorEntry(
    const std::string& name,
    float value,
    uint64_t timeStamp) {
  auto sensorEntry = SensorEntry();
  sensorEntry.name = name;
  sensorEntry.value = value;
  sensorEntry.lastUpdated = timeStamp;
  sensorEntry_[name] = sensorEntry;
}

void SensorData::setLastQsfpSvcTime(uint64_t t) {
  lastSuccessfulQsfpServiceContact_ = t;
}
uint64_t SensorData::getLastQsfpSvcTime() {
  return lastSuccessfulQsfpServiceContact_;
}

OpticEntry* SensorData::getOrCreateOpticEntry(const std::string& name) {
  bool entryExist = checkIfOpticEntryExists(name);
  OpticEntry* pEntry;
  if (!entryExist) {
    opticEntry_[name] = OpticEntry();
  }
  pEntry = &opticEntry_[name];
  return pEntry;
}

bool SensorData::checkIfOpticEntryExists(const std::string& name) const {
  return (opticEntry_.find(name) != opticEntry_.end());
}

OpticEntry* SensorData::getOpticEntry(const std::string& name) const {
  auto itr = opticEntry_.find(name);
  return itr == opticEntry_.end() ? nullptr
                                  : const_cast<OpticEntry*>(&(itr->second));
}

void SensorData::setOpticEntry(
    const std::string& name,
    std::vector<std::pair<std::string, float>> input,
    uint64_t timestamp) {
  auto pEntry = getOrCreateOpticEntry(name);
  pEntry->data = input;
  pEntry->lastOpticsUpdateTimeInSec = timestamp;
  pEntry->calculatedPwm = 0.0;
  pEntry->dataProcessTimeStamp = 0;
}

void SensorData::setOpticsPwm(const std::string& name, int v) {
  auto pEntry = getOpticEntry(name);
  if (pEntry == nullptr) {
    throw facebook::fboss::FbossError("Unable to find the optic data ", name);
  }
  pEntry->calculatedPwm = v;
}
int SensorData::getOpticsPwm(const std::string& name) {
  auto entry = getOpticEntry(name);
  if (entry == nullptr) {
    throw facebook::fboss::FbossError("Unable to find the optic data ", name);
  }
  return entry->calculatedPwm;
}

} // namespace facebook::fboss::platform::fan_service
