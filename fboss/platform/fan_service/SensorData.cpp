// Copyright 2021- Facebook. All rights reserved.

// Implementation of SensorData class. Refer to .h file
// for the functional description
#include "fboss/platform/fan_service/SensorData.h"

SensorData::SensorData() {}

SensorData::~SensorData() {}

SensorEntry* SensorData::getOrCreateSensorEntry(std::string name) {
  bool entryExist = checkIfEntryExists(name);
  SensorEntry* pEntry;
  if (!entryExist)
    sensorEntry_[name] = SensorEntry();
  pEntry = &sensorEntry_[name];
  return pEntry;
}

SensorEntry::~SensorEntry() {
  // Defining this to make unique_ptr happy
}

void SensorData::updateEntryInt(
    std::string name,
    int data,
    uint64_t timeStamp) {
  SensorEntry* pEntry = getOrCreateSensorEntry(name);
  pEntry->timeStampSec = timeStamp;
  // pEntry->odsTime=facebook::WallClockUtil::NowInSecFast();
  pEntry->sensorEntryType = kSensorEntryInt;
  pEntry->value.intValue = data;
  return;
}
void SensorData::updateEntryFloat(
    std::string name,
    float data,
    uint64_t timeStamp) {
  SensorEntry* pEntry = getOrCreateSensorEntry(name);
  pEntry->timeStampSec = timeStamp;
  pEntry->sensorEntryType = kSensorEntryFloat;
  pEntry->value.floatValue = data;
  return;
}
int SensorData::getSensorDataInt(std::string name) {
  SensorEntry* pEntry = getSensorEntry(name);
  if (pEntry == NULL)
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  if (pEntry->sensorEntryType != kSensorEntryInt)
    throw facebook::fboss::FbossError("Sensor Entry is not Int: ", name);
  return pEntry->value.intValue;
}

std::vector<std::string> SensorData::getKeyLists() {
  std::vector<std::string> retVal;
  retVal.reserve(sensorEntry_.size());
  for (auto kv : sensorEntry_) {
    retVal.push_back(kv.first);
  }
  return retVal;
}

SensorEntryType SensorData::getSensorEntryType(std::string name) {
  SensorEntry* pEntry = getSensorEntry(name);
  if (pEntry == NULL)
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  if (pEntry->sensorEntryType != kSensorEntryFloat)
    throw facebook::fboss::FbossError("Sensor Entry is not Float: ", name);
  return pEntry->sensorEntryType;
}

float SensorData::getSensorDataFloat(std::string name) {
  SensorEntry* pEntry = getSensorEntry(name);
  if (pEntry == NULL)
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  if (pEntry->sensorEntryType != kSensorEntryFloat)
    throw facebook::fboss::FbossError("Sensor Entry is not Float: ", name);
  return pEntry->value.floatValue;
}

uint64_t SensorData::getLastUpdated(std::string name) {
  SensorEntry* pEntry = getSensorEntry(name);
  if (pEntry == NULL)
    throw facebook::fboss::FbossError("Unable to find the sensor data ", name);
  return pEntry->timeStampSec;
}

bool SensorData::checkIfEntryExists(std::string name) {
  return (sensorEntry_.find(name) != sensorEntry_.end());
}

SensorEntry* SensorData::getSensorEntry(std::string name) {
  if (!checkIfEntryExists(name))
    return NULL;
  return &sensorEntry_[name];
}
