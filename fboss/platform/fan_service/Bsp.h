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
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/SensorServiceThrift.h"

// Facebook Service Router Headerfiles
// - used when sending Thrift request to sensor_service
#include "servicerouter/client/cpp2/ClientParams.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"
// Coroutine BlockWait headerfile
#include <folly/experimental/coro/BlockingWait.h>

// Includes for using raw thriftclient
#include <folly/SocketAddress.h>
#include <folly/Subprocess.h>
#include <folly/futures/Future.h>
#include <folly/io/async/AsyncSSLSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/SSLContext.h>
#include <folly/system/Shell.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "security/ca/lib/certpathpicker/CertPathPicker.h"

namespace facebook::fboss::platform {

constexpr int kSensorSendTimeoutMs = 5000;
constexpr int kSensorConnTimeoutMs = 2000;

class Bsp {
 public:
  virtual ~Bsp();
  // getSensorData: Get sensor data from either cache or direct access
  virtual void getSensorData(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData);
  // emergencyShutdown: function to shutdown the platform upon overheat
  virtual int emergencyShutdown(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      bool enable);
  int kickWatchdog(std::shared_ptr<ServiceConfig> pServiceConfig);
  // setFanPwm... : Fan pwm set function. Used by Control Logic Class
  bool setFanPwmSysfs(std::string path, int pwm);
  bool setFanPwmShell(std::string command, std::string fanName, int pwm);
  // setFanLed... : Fan pwm set function. Used by Control Logic Class
  bool setFanLedSysfs(std::string path, int pwm);
  bool setFanLedShell(std::string command, std::string fanName, int pwm);
  // Other public functions that cannot be overridden
  virtual uint64_t getCurrentTime() const;
  virtual bool checkIfInitialSensorDataRead() const;
  bool getEmergencyState() const;
  virtual float readSysfs(std::string path) const;
  // Methods for send thrift request without using servicerouter
  static folly::Future<
      std::unique_ptr<facebook::fboss::platform::sensor_service::
                          SensorServiceThriftAsyncClient>>
  createSensorServiceClient(folly::EventBase* eb);
  static apache::thrift::RpcOptions getRpcOptions();

 protected:
  // replaceAllString : String replace helper function
  std::string replaceAllString(
      std::string original,
      std::string src,
      std::string tgt) const;
  // This attribute is accessed by internal function and Mock class (Mokujin)
  void setEmergencyState(bool state);

 private:
  int run(const std::string& cmd);
  std::unique_ptr<std::thread> thread_{nullptr};
  folly::EventBase evb_;
  // For communicating with sensor_service
  folly::EventBase evbSensor_;
  bool emergencyShutdownState_{false};

  // Private Attributes
  int sensordThriftPort_{7001};
  bool initialSensorDataRead_{false};

  // Low level access function for setting PWM and LED value
  virtual bool writeSysfs(std::string path, int value);
  virtual bool setFanShell(
      std::string command,
      std::string valueSymbol,
      std::string fanName,
      int pwm);

  // Private Methods
  // Various handlers to fetch sensor data from Thrift / Utility / Rest / Sysfs
  void getSensorDataThrift(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData);
  void getSensorDataThriftWithSensorList(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData,
      std::vector<std::string> sensorList);
  void getSensorDataFb303WithSensorList(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData,
      std::vector<std::string> sensorList);
  void getSensorDataUtil(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData);
  float getSensorDataSysfs(std::string path);
  void getSensorDataRest(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData);
};

} // namespace facebook::fboss::platform
