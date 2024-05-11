// Copyright 2021- Facebook. All rights reserved.

// SensorData class is a part of Fan Service
// Role : To store sensor data and its metadata like timestamp,
//        and also to support key-based value search.

#pragma once

#include <unordered_map>
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform::fan_service {

struct SensorEntry {
 public:
  std::string name;
  uint64_t lastUpdated{0};
  float value{0};
};

struct OpticEntry {
  // Map from opticType to sensor reading
  std::vector<std::pair<std::string, float>> data;
  // Timestamp of this entry updated
  uint64_t lastOpticsUpdateTimeInSec{0};
  // Timestamp of the data, set by qsfp service
  // (used only for checking of qsfpcache is stale or not)
  uint64_t qsfpServiceTimeStamp{0};
  uint64_t dataProcessTimeStamp{0};
  int calculatedPwm{0};
};

class SensorData {
 public:
  std::optional<SensorEntry> getSensorEntry(const std::string& name) const;
  void updateSensorEntry(const std::string& name, float data, uint64_t ts);

  bool checkIfOpticEntryExists(const std::string& name) const;

  void setOpticEntry(
      const std::string& name,
      std::vector<std::pair<std::string, float>> input,
      uint64_t timestamp);
  OpticEntry* getOpticEntry(const std::string& name) const;
  OpticEntry* getOrCreateOpticEntry(const std::string& name);
  auto opticEntrySize() const {
    return opticEntry_.size();
  }
  const std::unordered_map<std::string, OpticEntry>& getOpticEntries() const {
    return opticEntry_;
  }

  std::unordered_map<std::string, SensorEntry> getSensorEntries() const {
    return sensorEntry_;
  }

  void setLastQsfpSvcTime(uint64_t t);
  uint64_t getLastQsfpSvcTime();

 private:
  std::unordered_map<std::string, SensorEntry> sensorEntry_;
  std::unordered_map<std::string, OpticEntry> opticEntry_;
  uint64_t lastSuccessfulQsfpServiceContact_;
};

} // namespace facebook::fboss::platform::fan_service
