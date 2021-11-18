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

#include <variant>

namespace facebook::fboss::platform {

// Fundamental data type definition
enum class SensorEntryType { kSensorEntryInt, kSensorEntryFloat };

// One sensor data entry
struct SensorEntry {
 public:
  std::string name;
  SensorEntryType sensorEntryType{SensorEntryType::kSensorEntryFloat};
  uint64_t timeStampSec = 0;
  std::variant<int, float> value{0};
};

// The main class for storing sensor data
class SensorData {
 public:
  // Get sensor data and its type
  int getSensorDataInt(const std::string& name) const;
  float getSensorDataFloat(const std::string& name) const;
  SensorEntryType getSensorEntryType(const std::string& name) const;
  // When was this sensor reading acquired?
  uint64_t getLastUpdated(const std::string& name) const;
  // Check if key exists in sensordata hash table
  bool checkIfEntryExists(const std::string& name) const;
  // Update entry
  void updateEntryInt(const std::string& name, int data, uint64_t timeStampSec);
  void
  updateEntryFloat(const std::string& name, float data, uint64_t timeStampSec);
  auto begin() const {
    return sensorEntry_.begin();
  }
  auto end() const {
    return sensorEntry_.end();
  }

 private:
  std::unordered_map<std::string, SensorEntry> sensorEntry_;
  const SensorEntry* getSensorEntry(const std::string& name) const;
  SensorEntry* getOrCreateSensorEntry(const std::string& name);
};

} // namespace facebook::fboss::platform
