// Copyright 2021- Facebook. All rights reserved.

// Implementation of FanServiceImpl class. Refer to .h file
// for functional description
#include "fboss/platform/fan_service/FanServiceImpl.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <stdexcept>

#include "common/time/Time.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/Utils.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {
auto constexpr kDefaultSensorReadFrequencyInSec = 30;
auto constexpr kDefaultFanControlFrequencyInSec = 30;
} // namespace

namespace facebook::fboss::platform::fan_service {

FanServiceImpl::FanServiceImpl(const std::string& configFile)
    : confFileName_(configFile) {
  lastControlExecutionTimeSec_ = 0;
  lastSensorFetchTimeSec_ = 0;

  pBsp_ = nullptr;
  pSensorData_ = nullptr;

  return;
}

unsigned int FanServiceImpl::getControlFrequency() const {
  return kDefaultFanControlFrequencyInSec;
}

unsigned int FanServiceImpl::getSensorFetchFrequency() const {
  return kDefaultSensorReadFrequencyInSec;
}

std::shared_ptr<Bsp> FanServiceImpl::BspFactory() {
  Bsp* returnVal = nullptr;
  switch (*config_.bspType()) {
    // In many cases, generic BSP is enough.
    case BspType::kBspGeneric:
    case BspType::kBspDarwin:
    case BspType::kBspLassen:
    case BspType::kBspMinipack3:
      returnVal = new Bsp(config_);
      break;
    // For unit testing, we use Mock (Mokujin) BSP.
    case BspType::kBspMokujin:
      returnVal = static_cast<Bsp*>(new Mokujin(config_));
      break;

    default:
      facebook::fboss::FbossError("Invalid BSP Type given to BSP Factory!");
      break;
  }
  std::shared_ptr<Bsp> returnValShared(returnVal);
  return returnValShared;
}

void FanServiceImpl::kickstart() {
  // Read Config
  std::string fanServiceConfJson;
  if (confFileName_.empty()) {
    XLOG(INFO) << "No config file was provided. Inferring from config_lib";
    fanServiceConfJson = ConfigLib().getFanServiceConfig();
  } else {
    XLOG(INFO) << "Using config file: " << confFileName_;
    if (!folly::readFile(confFileName_.c_str(), fanServiceConfJson)) {
      throw std::runtime_error(
          "Can not find sensor config file: " + confFileName_);
    }
  }

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
}

int FanServiceImpl::controlFan(/*folly::EventBase* evb*/) {
  int rc = 0;
  uint64_t currentTimeSec = pBsp_->getCurrentTime();
  if (!transitionValueSet_) {
    transitionValueSet_ = true;
    XLOG(INFO)
        << "Upon fan_service start up, program all fan pwm with transitional value of "
        << *config_.pwmTransitionValue();
    pControlLogic_->setTransitionValue();
  }
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

int FanServiceImpl::runMock(
    std::string mockInputFile,
    std::string mockOutputFile) {
  int rc = 0;
  uint64_t timeToAdvanceSec = 0, currentTimeSec = 0;
  bool loopControl = true;
  std::string simulationSensorName;
  float simulationSensorValue;
  // Make sure BSP is a mock bsp type
  if (*config_.bspType() != BspType::kBspMokujin) {
    XLOG(ERR) << "Mock mode is enabled, but BSP is not a Mock BSP!";
    return -1;
  }
  Mokujin* pMokujin = static_cast<Mokujin*>(pBsp_.get());
  pMokujin->openIOFiles(mockInputFile, mockOutputFile);

  while (loopControl) {
    // Read and Parse Mock data as needed
    XLOG(INFO) << "Current Time : " << currentTimeSec;
    pMokujin->setTimeStamp(currentTimeSec);
    while ((!pMokujin->isEof()) &&
           (pMokujin->hasAnyMoreEvent(currentTimeSec))) {
      XLOG(INFO) << "Processing system state change event.";
      pMokujin->getNextSensorEvent(
          currentTimeSec, simulationSensorName, simulationSensorValue);
      XLOG(INFO) << "Read System Event for " << simulationSensorName
                 << " set to " << simulationSensorValue;
      if (simulationSensorName == "end") {
        XLOG(INFO) << "No more event data. Time to end the simulation";
        loopControl = false;
        break;
      } else if (simulationSensorName != "") {
        pMokujin->updateSimulationData(
            simulationSensorName, simulationSensorValue);
        XLOG(INFO) << "Updated system status.";
      }
    }
    XLOG(INFO)
        << "Done changing system state according to simulation scenario.";
    // Read sensor if needed
    if ((currentTimeSec - lastSensorFetchTimeSec_) >= getControlFrequency()) {
      lastSensorFetchTimeSec_ = currentTimeSec;
      XLOG(INFO) << "Time to read sensor data";
      pBsp_->getSensorData(pSensorData_);
      XLOG(INFO) << "Done reading sensor data";
    }
    // Update fan as needed
    if ((currentTimeSec - lastControlExecutionTimeSec_) >=
        getControlFrequency()) {
      lastControlExecutionTimeSec_ = currentTimeSec;
      XLOG(INFO) << "Time for running fan control logic";
      pControlLogic_->updateControl(pSensorData_);
      pBsp_->getOpticsData(pSensorData_);
      XLOG(INFO) << "Done adjusting fan speed";
    }

    // We still need to emulate the watchdog,
    // but kicking it every 5 seconds will fill up the simulation data
    // with all the garbage outputs. So we do it only once per next
    // event calculation (still too much. try not to use watchdog
    // in simulation config, as it will generate too much bogus
    // output)
    pBsp_->kickWatchdog();

    // Figure out when is the next event,
    // so that we can jump to that time in the future.
    timeToAdvanceSec = lastSensorFetchTimeSec_ + getSensorFetchFrequency();
    if (timeToAdvanceSec >
        lastControlExecutionTimeSec_ + getControlFrequency()) {
      timeToAdvanceSec = lastControlExecutionTimeSec_ + getControlFrequency();
    }
    if (timeToAdvanceSec > pMokujin->getNextEventTime()) {
      timeToAdvanceSec = pMokujin->getNextEventTime();
    }
    if (timeToAdvanceSec < currentTimeSec + 1) {
      timeToAdvanceSec = currentTimeSec + 1;
    }
    // Update current time with the next event time (future)
    currentTimeSec = timeToAdvanceSec;
  }
  XLOG(INFO) << "Done running simulation.";
  pMokujin->closeFiles();
  XLOG(INFO) << "File closed. Exiting...";
  return rc;
}
} // namespace facebook::fboss::platform::fan_service
