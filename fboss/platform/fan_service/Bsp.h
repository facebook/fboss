// Copyright 2021- Facebook. All rights reserved.

// BSP Base Class, a part of Fan Service
//
// Role : Take care of all the I/O and other board specific details
//        such as, emergency_shutdown, sysfs read, thrift read, util read
//        A base class where it can be extended by platform specific class.
#pragma once

#include <cstdint>
#include <fstream>
#include <sstream>

#include <folly/coro/BlockingWait.h>
#include <folly/io/async/EventBase.h>
#include <folly/system/Shell.h>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/platform/fan_service/DataFetcher.h"
#include "fboss/platform/fan_service/FsdbSensorSubscriber.h"
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_state_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_stats_types.h"

namespace facebook::fboss::platform::fan_service {

class Bsp {
 public:
  explicit Bsp(const FanServiceConfig& config);
  virtual ~Bsp();
  // getSensorData: Get sensor data from either cache or direct access
  virtual void getSensorData(std::shared_ptr<SensorData> pSensorData);
  // getOpticsData: Get Optics temperature data
  virtual void getOpticsData(std::shared_ptr<SensorData> pSensorData);
  // emergencyShutdown: function to shutdown the platform upon overheat
  virtual int emergencyShutdown(bool enable);
  int kickWatchdog();
  virtual bool setFanPwmSysfs(const std::string& path, int pwm);
  virtual bool setFanLedSysfs(const std::string& path, int pwm);
  virtual uint64_t getCurrentTime() const;
  virtual bool checkIfInitialSensorDataRead() const;
  bool getEmergencyState() const;
  virtual float readSysfs(const std::string& path) const;
  static apache::thrift::RpcOptions getRpcOptions();

  FsdbSensorSubscriber* fsdbSensorSubscriber() {
    return fsdbSensorSubscriber_.get();
  }
  void getSensorDataThrift(std::shared_ptr<SensorData> pSensorData);

 protected:
  // This attribute is accessed by internal function.
  void setEmergencyState(bool state);

  const FanServiceConfig config_;

 private:
  void getOpticsDataFromQsfpSvc(
      const Optic& opticsGroup,
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
  virtual bool writeSysfs(const std::string& path, int value);
  std::vector<std::pair<std::string, float>> processOpticEntries(
      const Optic& opticsGroup,
      std::shared_ptr<SensorData> pSensorData,
      uint64_t& currentQsfpSvcTimestamp,
      const std::map<int32_t, TransceiverInfo>& transceiverInfoMap);

  std::unique_ptr<FsdbSensorSubscriber> fsdbSensorSubscriber_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
};
} // namespace facebook::fboss::platform::fan_service
