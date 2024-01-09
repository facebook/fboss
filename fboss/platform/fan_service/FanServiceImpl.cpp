// Copyright 2021- Facebook. All rights reserved.

#include "fboss/platform/fan_service/FanServiceImpl.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "common/time/Time.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/Utils.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {
auto constexpr kDefaultSensorReadFrequencyInSec = 30;
auto constexpr kDefaultFanControlFrequencyInSec = 30;
} // namespace

namespace facebook::fboss::platform::fan_service {

FanServiceImpl::FanServiceImpl() {
  std::string fanServiceConfJson = ConfigLib().getFanServiceConfig();
  apache::thrift::SimpleJSONSerializer::deserialize<FanServiceConfig>(
      fanServiceConfJson, config_);
  XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      config_);
  if (!Utils().isValidConfig(config_)) {
    XLOG(ERR) << "Invalid config! Aborting...";
    throw std::runtime_error("Invalid Config.  Aborting...");
  }

  // Get the proper BSP object from BSP factory,
  // according to the parsed config, then run init routine.
  pBsp_ = BspFactory();
  // Create and initialize QsfpCache object. This may take
  // some time.
  pBsp_->initializeQsfpService();

  // Initialize SensorData
  pSensorData_ = std::make_shared<SensorData>();
  pSensorData_->setLastQsfpSvcTime(pBsp_->getCurrentTime());

  // Start control logic, and attach bsp and sensors
  pControlLogic_ = std::make_shared<ControlLogic>(config_, pBsp_);
  // Set the fans with transitional pwm value.
  XLOG(INFO)
      << "Upon fan_service start up, program all fan pwm with transitional value of "
      << *config_.pwmTransitionValue();
  pControlLogic_->setTransitionValue();
}

unsigned int FanServiceImpl::getControlFrequency() const {
  return kDefaultFanControlFrequencyInSec;
}

unsigned int FanServiceImpl::getSensorFetchFrequency() const {
  return kDefaultSensorReadFrequencyInSec;
}

std::shared_ptr<Bsp> FanServiceImpl::BspFactory() {
  return std::make_shared<Bsp>(config_);
}

int FanServiceImpl::controlFan(/*folly::EventBase* evb*/) {
  int rc = 0;
  uint64_t currentTimeSec = pBsp_->getCurrentTime();
  // Update Sensor Value based according to fetch frequency
  if ((currentTimeSec - lastSensorFetchTimeSec_) >= getSensorFetchFrequency()) {
    bool sensorReadOK = false;
    bool opticsReadOK = false;

    // Get the updated sensor data
    try {
      pBsp_->getSensorData(pSensorData_);
      sensorReadOK = true;
      XLOG(INFO) << "Successfully fetched sensor data.";
    } catch (std::exception& e) {
      XLOG(ERR) << "Failed to get sensor data with error : " << e.what();
    }

    // Also get the updated optics data
    try {
      // Get the updated optics data
      pBsp_->getOpticsData(pSensorData_);
      opticsReadOK = true;
      XLOG(INFO) << "Successfully fetched optics data.";
    } catch (std::exception& e) {
      XLOG(ERR) << "Failed to get optics data with error : " << e.what();
    }
    // If BOTH of the two data read above pass, then we consider
    // that the sensor reading is successful. Otherwise, we will
    // keep retrying.
    if (sensorReadOK && opticsReadOK) {
      lastSensorFetchTimeSec_ = currentTimeSec;
    }
  }
  // Change Fan PWM as needed according to control execution frequency
  if ((currentTimeSec - lastControlExecutionTimeSec_) >=
      getControlFrequency()) {
    lastControlExecutionTimeSec_ = currentTimeSec;
    pControlLogic_->updateControl(pSensorData_);
  }

  pBsp_->kickWatchdog();

  return rc;
}

} // namespace facebook::fboss::platform::fan_service
