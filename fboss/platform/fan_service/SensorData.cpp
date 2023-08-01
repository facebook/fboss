// Copyright 2021- Facebook. All rights reserved.

// Implementation of SensorData class. Refer to .h file
// for the functional description
#include "fboss/platform/fan_service/SensorData.h"
// Additional FB helper funtion
#include "common/time/Time.h"

namespace facebook::fboss::platform::fan_service {

void SensorData::setLastQsfpSvcTime(uint64_t t) {
  lastSuccessfulQsfpServiceContact_ = t;
}
uint64_t SensorData::getLastQsfpSvcTime() {
  return lastSuccessfulQsfpServiceContact_;
}

SensorEntry* SensorData::getOrCreateSensorEntry(const std::string& name) {
  bool entryExist = checkIfEntryExists(name);
  SensorEntry* pEntry;
  if (!entryExist) {
    sensorEntry_[name] = SensorEntry();
  }
  pEntry = &sensorEntry_[name];
  return pEntry;
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

void SensorData::updateEntryInt(
    const std::string& name,
    int data,
    uint64_t timeStamp) {
  auto pEntry = getOrCreateSensorEntry(name);
  pEntry->timeStampSec = timeStamp;
  // pEntry->odsTime=facebook::WallClockUtil::NowInSecFast();
  pEntry->sensorEntryType = SensorEntryType::kSensorEntryInt;
  pEntry->value = data;
}
void SensorData::updateEntryFloat(
    const std::string& name,
    float data,
    uint64_t timeStamp) {
  auto pEntry = getOrCreateSensorEntry(name);
  pEntry->timeStampSec = timeStamp;
  pEntry->sensorEntryType = SensorEntryType::kSensorEntryFloat;
  pEntry->value = data;
}

int SensorData::getSensorDataInt(const std::string& name) const {
  auto pEntry = getSensorEntry(name);
  if (pEntry == nullptr) {
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  }
  if (pEntry->sensorEntryType != SensorEntryType::kSensorEntryInt) {
    throw facebook::fboss::FbossError("Sensor Entry is not Int: ", name);
  }
  return std::get<int>(pEntry->value);
}

SensorEntryType SensorData::getSensorEntryType(const std::string& name) const {
  auto pEntry = getSensorEntry(name);
  if (pEntry == nullptr) {
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  }
  if (pEntry->sensorEntryType != SensorEntryType::kSensorEntryFloat) {
    throw facebook::fboss::FbossError("Sensor Entry is not Float: ", name);
  }
  return pEntry->sensorEntryType;
}

float SensorData::getSensorDataFloat(const std::string& name) const {
  auto pEntry = getSensorEntry(name);
  if (pEntry == nullptr) {
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  }
  if (pEntry->sensorEntryType != SensorEntryType::kSensorEntryFloat) {
    throw facebook::fboss::FbossError("Sensor Entry is not Float: ", name);
  }
  return std::get<float>(pEntry->value);
}

uint64_t SensorData::getLastUpdated(const std::string& name) const {
  auto pEntry = getSensorEntry(name);
  if (pEntry == nullptr) {
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  }
  return pEntry->timeStampSec;
}

bool SensorData::checkIfEntryExists(const std::string& name) const {
  return (sensorEntry_.find(name) != sensorEntry_.end());
}

bool SensorData::checkIfOpticEntryExists(const std::string& name) const {
  return (opticEntry_.find(name) != opticEntry_.end());
}

const SensorEntry* SensorData::getSensorEntry(const std::string& name) const {
  auto itr = sensorEntry_.find(name);
  return itr == sensorEntry_.end() ? nullptr : &(itr->second);
}

OpticEntry* SensorData::getOpticEntry(const std::string& name) const {
  auto itr = opticEntry_.find(name);
  return itr == opticEntry_.end() ? nullptr
                                  : const_cast<OpticEntry*>(&(itr->second));
}

void SensorData::setOpticEntry(
    const std::string& name,
    std::vector<std::pair<OpticTableType, float>> input,
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
