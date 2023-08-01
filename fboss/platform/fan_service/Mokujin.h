// Copyright 2021- Facebook. All rights reserved.
#pragma once

#include "Bsp.h"

// Mokujin : A Mock class for running unit test and simulations.
//           Implemented by extending Bsp class, and will work like Bsp class.
//           When we run unit tests, we replace BSP with this Mock clas
//           So that we use the same fan service logic while we replace
//           the input with test scenario file, and the output with
//           test output file describing when pwm changes and so on.

namespace facebook::fboss::platform::fan_service {
class Mokujin : public Bsp {
 public:
  explicit Mokujin(const FanServiceConfig& config);
  ~Mokujin();
  // Override some methods in Bsp class
  void getSensorData(std::shared_ptr<SensorData> pSensorData) override;
  int emergencyShutdown(bool enable) override;
  uint64_t getCurrentTime() const override;
  void getOpticsData(std::shared_ptr<SensorData> pSensorData) override;

  // The following public methods are used by Fan Service to interact with
  // this Mock DSP, so that it will timewarp when there is no more current
  // event pending
  bool bothFileOpen();
  int getNextEventTime();
  bool hasAnyMoreEvent(uint64_t timeInSec);
  bool getNextSensorEvent(uint64_t timeSec, std::string& key, float& value);
  bool isEof();
  void setTimeStamp(uint64_t stp);
  uint64_t getTimeStamp();
  void updateSimulationData(std::string key, float value);
  void closeFiles();
  void openIOFiles(std::string iFileName, std::string oFileName);
  float readSysfs(std::string path) const override;
  bool initializeQsfpService() override;

 private:
  // Attributes, mostly for keeping the state of the simulation
  uint64_t currentTimeStampSec_{0};
  std::string nextEventSensor_;
  float nextEventValue_;
  uint64_t nextEventTimeSec_;
  std::unordered_map<std::string, float> simulatedSensorRead_;
  std::string inputFileName_;
  std::string outputFileName_;
  std::ifstream iFs_;
  bool iFOpen_{false};
  bool eofIf_;
  bool nextEventDeadSensor_{false};
  std::ofstream oFs_;
  bool oFOpen_{false};
  bool writeSysfs(std::string path, int value) override;
  bool setFanShell(
      std::string command,
      std::string keySymbol,
      std::string fanName,
      int pwm) override;
  int run(const std::string& cmd) override;
};
} // namespace facebook::fboss::platform::fan_service
