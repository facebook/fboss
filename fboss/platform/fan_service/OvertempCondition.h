// Copyright 2025- Facebook. All rights reserved.

#pragma once
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_types.h"
namespace facebook::fboss::platform::fan_service {

class OvertempCondition {
  struct OvertempConditionEntry {
    std::string sensorName;
    float overtempThreshold{0};
    int slidingWindowSize{1};
    std::vector<float> sensorReads;
    int cursor{0};
    int length{0};
    float sumTotal{0};
  };

 public:
  void setupShutdownConditions(const FanServiceConfig& config);
  void processSensorData(std::string sensorName, float sensorValue);
  void addSensorForTracking(std::string name, float threshold, int windowSize);
  bool checkIfOvertemp();
  bool isTracked(const std::string);
  void setNumOvertempSensorForShutdown(int number);

 private:
  std::map<std::string, OvertempConditionEntry> entries;
  int numOvertempForShutdown = 0;
};

} // namespace facebook::fboss::platform::fan_service
