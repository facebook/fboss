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
#include <cstdint>
#include <fstream>
#include <sstream>

// Header file of the other classes used by this class
#include "SensorData.h"
#include "ServiceConfig.h"

// Auto-generated Thrift inteface headerfile (by Buck)
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"

// Coroutine BlockWait headerfile
#include <folly/experimental/coro/BlockingWait.h>

#include <folly/io/async/EventBase.h>
#include <folly/system/Shell.h>
#include "fboss/platform/fan_service/HelperFunction.h"

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/platform/fan_service/FsdbSensorSubscriber.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"

namespace {
struct TransceiverData {
  int portID;
  facebook::fboss::GlobalSensors sensor;
  facebook::fboss::MediaInterfaceCode mediaInterfaceCode;
  uint32_t timeCollected;

  TransceiverData(
      int portID_,
      facebook::fboss::GlobalSensors sensor_,
      facebook::fboss::MediaInterfaceCode mediaInterfaceCode_,
      uint32_t timeCollected_) {
    portID = portID_;
    sensor = sensor_;
    mediaInterfaceCode = mediaInterfaceCode_;
    timeCollected = timeCollected_;
  }
};
} // namespace

namespace facebook::fboss::platform {

class Bsp {
 public:
  Bsp();
  virtual ~Bsp();
  // getSensorData: Get sensor data from either cache or direct access
  virtual void getSensorData(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData);
  // getOpticsData: Get Optics temperature data
  virtual void getOpticsData(
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
  virtual bool initializeQsfpService();
  static apache::thrift::RpcOptions getRpcOptions();

  FsdbSensorSubscriber* fsdbSensorSubscriber() {
    return fsdbSensorSubscriber_.get();
  }
  void getSensorDataThrift(
      std::shared_ptr<ServiceConfig> pServiceConfig,
      std::shared_ptr<SensorData> pSensorData);

 protected:
  // replaceAllString : String replace helper function
  std::string replaceAllString(
      std::string original,
      std::string src,
      std::string tgt) const;
  // This attribute is accessed by internal function and Mock class (Mokujin)
  void setEmergencyState(bool state);

 private:
  virtual int run(const std::string& cmd);
  void getOpticsDataThrift(
      Optic* opticsGroup,
      std::shared_ptr<SensorData> pSensorData);
  void getOpticsDataSysfs(
      Optic* opticsGroup,
      std::shared_ptr<SensorData> pSensorData);
  std::shared_ptr<std::thread> thread_{nullptr};
  // For communicating with qsfp_service
  folly::EventBase evb_;
  // For communicating with sensor_service
  folly::EventBase evbSensor_;
  bool emergencyShutdownState_{false};

  // Private Attributes
  int sensordThriftPort_{5970};
  int qsfpSvcThriftPort_{5910};
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
  void processOpticEntries(
      Optic* opticsGroup,
      std::shared_ptr<SensorData> pSensorData,
      uint64_t& currentQsfpSvcTimestamp,
      const std::map<int32_t, TransceiverData>& cacheTable,
      OpticEntry* opticData);

  std::unique_ptr<FsdbSensorSubscriber> fsdbSensorSubscriber_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  folly::Synchronized<
      std::map<std::string, fboss::platform::sensor_service::SensorData>>
      subscribedSensorData;
};
} // namespace facebook::fboss::platform
