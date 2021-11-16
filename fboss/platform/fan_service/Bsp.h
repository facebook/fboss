// Copyright 2021- Facebook. All rights reserved.

// BSP Base Class, a part of Fan Service
//
// Role : Take care of all the I/O and other board specific details
//        such as, emergency_shutdown, sysfs read, thrift read, util read
//        A base class where it can be extended by platform specific class or
//        overridden by mock Bsp (Mokujin)

// Don't parse multiple times
#pragma once

// Standard CPP include file
#include <fstream>
#include <sstream>

// Header file of the other classes used by this class
#include "SensorData.h"
#include "ServiceConfig.h"

// Auto-generated Thrift inteface headerfile (by Buck)
#include "fboss/platform/sensor_service/if/gen-cpp2/SensorService.h"

// Facebook Service Router Headerfiles
// - used when sending Thrift request to sensor_service
#include "servicerouter/client/cpp2/ClientParams.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"
// Coroutine BlockWait headerfile
#include <folly/experimental/coro/BlockingWait.h>
namespace facebook::fboss::platform {
class Bsp {
 public:
  virtual ~Bsp() = default;
  // getSensorData: Get sensor data from either cache or direct access
  virtual void getSensorData(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData);
  // emergencyShutdown: function to shutdown the platform upon overheat
  virtual int emergencyShutdown(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      bool enable);
  // setFanPwm... : Fan pwm set function. Used by Control Logic Class
  virtual bool setFanPwmSysfs(std::string path, int pwm);
  virtual bool
  setFanPwmShell(std::string command, std::string fanName, int pwm);
  // Other public functions that cannot be overridden
  virtual uint64_t getCurrentTime() const;
  virtual bool checkIfInitialSensorDataRead() const;
  bool getEmergencyState() const;
  float readSysfs(std::string path) const;

 protected:
  // replaceAllString : String replace helper function
  std::string replaceAllString(
      std::string original,
      std::string src,
      std::string tgt) const;
  // This attribute is accessed by internal function and Mock class (Mokujin)
  void setEmergencyState(bool state);

 private:
  bool emergencyShutdownState_{false};

  // Private Attributes
  int sensordThriftPort_{7001};
  bool initialSensorDataRead_{false};

  // Private Methods
  // Various handlers to fetch sensor data from Thrift / Utility / Rest / Sysfs
  void getSensorDataThrift(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData) const;
  void getSensorDataThriftWithSensorList(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData,
      std::vector<std::string> sensorList) const;
  void getSensorDataUtil(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData) const;
  float getSensorDataSysfs(std::string path) const;
  void getSensorDataRest(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData) const;
};

} // namespace facebook::fboss::platform
