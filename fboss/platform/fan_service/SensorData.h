// Copyright 2021- Facebook. All rights reserved.

// SensorData class is a part of Fan Service
// Role : To store sensor data and its metadata like timestamp,
//        and also to support key-based value search.

#pragma once
// Standard C++ routines
#include <unordered_map>
#include <vector>
// Additional FB helper funtion
#include "common/time/Time.h"
// We reuse the same facebook::fboss::FbossError
// , for consistency
#include "fboss/agent/FbossError.h"

namespace facebook::fboss::platform {

// Fundamental data type definition
typedef enum { kSensorEntryInt, kSensorEntryFloat } SensorEntryType;

// One sensor data entry
class SensorEntry {
 public:
  union Value {
    float floatValue;
    int intValue;
  };
  std::string name;
  SensorEntryType sensorEntryType;
  SensorEntry() {
    sensorEntryType = kSensorEntryFloat;
    timeStampSec = 0;
    value.floatValue = 0.0;
  }
  ~SensorEntry();
  uint64_t timeStampSec;
  Value value;
};

// The main class for storing sensor data
class SensorData {
 public:
  // Constructor / Destructor
  SensorData();
  ~SensorData();
  // Get sensor data and its type
  int getSensorDataInt(std::string name);
  float getSensorDataFloat(std::string name);
  SensorEntryType getSensorEntryType(std::string name);
  // When was this sensor reading acquired?
  uint64_t getLastUpdated(std::string name);
  // Check if key exists in sensordata hash table
  bool checkIfEntryExists(std::string name);
  // Update entry
  void updateEntryInt(std::string name, int data, uint64_t timeStampSec);
  void updateEntryFloat(std::string name, float data, uint64_t timeStampSec);
  // Get key lists of the hash table for iteration (during ODS streaming)
  std::vector<std::string> getKeyLists();

 private:
  std::unordered_map<std::string, SensorEntry> sensorEntry_;
  SensorEntry* getSensorEntry(std::string name);
  SensorEntry* getOrCreateSensorEntry(std::string name);
};

} // namespace facebook::fboss::platform
