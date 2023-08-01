// Copyright 2021- Facebook. All rights reserved.

// SensorData class is a part of Fan Service
// Role : To store sensor data and its metadata like timestamp,
//        and also to support key-based value search.

#pragma once
// Standard C++ routines
#include <unordered_map>
#include <vector>
// We reuse the same facebook::fboss::FbossError
// , for consistency
#include "fboss/agent/FbossError.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

#include <variant>

namespace facebook::fboss::platform::fan_service {

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

struct OpticEntry {
  // Last successful optics update time. Used for boost mode.
  std::string name;
  std::vector<std::pair<OpticTableType, float>> data;
  // Timestamp of this entry updated
  uint64_t lastOpticsUpdateTimeInSec{0};
  // Timestamp of the data, set by qsfp service
  // (used only for checking of qsfpcache is stale or not)
  uint64_t qsfpServiceTimeStamp{0};
  uint64_t dataProcessTimeStamp{0};
  int calculatedPwm{0};
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
  bool checkIfOpticEntryExists(const std::string& name) const;
  // Update entry
  void updateEntryInt(const std::string& name, int data, uint64_t timeStampSec);
  void
  updateEntryFloat(const std::string& name, float data, uint64_t timeStampSec);
  void setOpticEntry(
      const std::string& name,
      std::vector<std::pair<OpticTableType, float>> input,
      uint64_t timestamp);
  /*
  std::vector<std::pair<OpticTableType,float>>
  getOpticEntry(std::string& name) const;
  */
  OpticEntry* getOpticEntry(const std::string& name) const;
  OpticEntry* getOrCreateOpticEntry(const std::string& name);
  void setOpticsPwm(const std::string& name, int v);
  int getOpticsPwm(const std::string& name);
  auto begin() const {
    return sensorEntry_.begin();
  }
  auto end() const {
    return sensorEntry_.end();
  }
  auto size() const {
    return sensorEntry_.size();
  }
  auto opticEntrySize() const {
    return opticEntry_.size();
  }
  const std::unordered_map<std::string, OpticEntry>& getOpticEntries() const {
    return opticEntry_;
  }

  void setLastQsfpSvcTime(uint64_t t);
  uint64_t getLastQsfpSvcTime();

 private:
  std::unordered_map<std::string, SensorEntry> sensorEntry_;
  std::unordered_map<std::string, OpticEntry> opticEntry_;
  const SensorEntry* getSensorEntry(const std::string& name) const;
  SensorEntry* getOrCreateSensorEntry(const std::string& name);
  uint64_t lastSuccessfulQsfpServiceContact_;
};

} // namespace facebook::fboss::platform::fan_service
