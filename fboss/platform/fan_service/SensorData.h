// Copyright 2021- Facebook. All rights reserved.

// SensorData class is a part of Fan Service
// Role : To store sensor data and its metadata like timestamp,
//        and also to support key-based value search.

#pragma once

#include <unordered_map>
#include <vector>

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform::fan_service {

struct SensorEntry {
 public:
  std::string name;
  uint64_t lastUpdated{0};
  float value{0};
};

struct OpticEntry {
  // Map from opticType to sensor (temperature) reading
  std::vector<std::pair<std::string, float>> data;
  // Timestamp of when this entry was last updated in fan_service
  uint64_t lastOpticsUpdateTimeInSec{0};
  // Timestamp of when this entry was last updated in qsfp_service
  uint64_t qsfpServiceTimeStamp{0};
  // Timestamp of when this entry was last processed by fan_service
  uint64_t dataProcessTimeStamp{0};
};

class SensorData {
 public:
  std::optional<SensorEntry> getSensorEntry(const std::string& name) const;
  void updateSensorEntry(const std::string& name, float data, uint64_t ts);
  void delSensorEntry(const std::string& name);

  std::optional<OpticEntry> getOpticEntry(const std::string& name) const;
  void updateOpticEntry(
      const std::string& name,
      const std::vector<std::pair<std::string, float>>& data,
      uint64_t qsfpServiceTimeStamp);
  void resetOpticData(const std::string& name);
  void updateOpticDataProcessingTimestamp(const std::string& name, uint64_t ts);

  const std::unordered_map<std::string, OpticEntry>& getOpticEntries() const {
    return opticEntries_;
  }

  std::unordered_map<std::string, SensorEntry> getSensorEntries() const {
    return sensorEntries_;
  }

  uint64_t getLastQsfpSvcTime();

 private:
  std::unordered_map<std::string, SensorEntry> sensorEntries_;
  std::unordered_map<std::string, OpticEntry> opticEntries_;
  uint64_t lastSuccessfulQsfpServiceContact_;
};

} // namespace facebook::fboss::platform::fan_service
