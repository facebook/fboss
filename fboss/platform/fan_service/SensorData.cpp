// Copyright 2021- Facebook. All rights reserved.

#include "fboss/platform/fan_service/SensorData.h"

#include "common/time/Time.h"

namespace facebook::fboss::platform::fan_service {

std::optional<SensorEntry> SensorData::getSensorEntry(
    const std::string& name) const {
  auto itr = sensorEntries_.find(name);
  if (itr == sensorEntries_.end()) {
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
  sensorEntries_[name] = sensorEntry;
}

void SensorData::delSensorEntry(const std::string& name) {
  sensorEntries_.erase(name);
}

std::optional<OpticEntry> SensorData::getOpticEntry(
    const std::string& name) const {
  auto itr = opticEntries_.find(name);
  if (itr == opticEntries_.end()) {
    return std::nullopt;
  }
  return itr->second;
}

void SensorData::updateOpticEntry(
    const std::string& name,
    const std::vector<std::pair<std::string, float>>& data,
    uint64_t qsfpServiceTimeStamp) {
  if (opticEntries_.find(name) == opticEntries_.end()) {
    opticEntries_[name] = OpticEntry();
  }
  opticEntries_[name].data = data;
  opticEntries_[name].qsfpServiceTimeStamp = qsfpServiceTimeStamp;
  opticEntries_[name].lastOpticsUpdateTimeInSec = WallClockUtil::NowInSecFast();
  lastSuccessfulQsfpServiceContact_ = qsfpServiceTimeStamp;
}

void SensorData::resetOpticData(const std::string& name) {
  if (opticEntries_.find(name) == opticEntries_.end()) {
    opticEntries_[name] = OpticEntry();
  }
  opticEntries_[name].data = {};
}

void SensorData::updateOpticDataProcessingTimestamp(
    const std::string& name,
    uint64_t ts) {
  if (opticEntries_.find(name) == opticEntries_.end()) {
    opticEntries_[name] = OpticEntry();
  }
  opticEntries_[name].dataProcessTimeStamp = ts;
}

uint64_t SensorData::getLastQsfpSvcTime() {
  return lastSuccessfulQsfpServiceContact_;
}

} // namespace facebook::fboss::platform::fan_service
