// Copyright 2021- Facebook. All rights reserved.

// Implementation of FanService class. Refer to .h file
// for functional description
#include "fboss/platform/fan_service/FanService.h"

#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"
// Additional FB helper funtion
#include "common/time/Time.h"

namespace {
auto constexpr kDefaultSensorReadFrequencyInSec = 30;
auto constexpr kDefaultFanControlFrequencyInSec = 30;
} // namespace

namespace facebook::fboss::platform {
FanService::FanService() {
  lastControlExecutionTimeSec_ = 0;
  lastSensorFetchTimeSec_ = 0;

  pBsp_ = NULL;
  pSensorData_ = NULL;

  return;
}

unsigned int FanService::getControlFrequency() const {
  return kDefaultFanControlFrequencyInSec;
}

unsigned int FanService::getSensorFetchFrequency() const {
  return kDefaultSensorReadFrequencyInSec;
}

std::shared_ptr<Bsp> FanService::BspFactory() {
  Bsp* returnVal = NULL;
  switch (pConfig_->bspType) {
    // In many cases, generic BSP is enough.
    case fan_config_structs::BspType::kBspGeneric:
    case fan_config_structs::BspType::kBspDarwin:
    case fan_config_structs::BspType::kBspLassen:
    case fan_config_structs::BspType::kBspMinipack3:
      returnVal = new Bsp();
      break;
    // For unit testing, we use Mock (Mokujin) BSP.
    case fan_config_structs::BspType::kBspMokujin:
      returnVal = static_cast<Bsp*>(new Mokujin());
      break;

    default:
      facebook::fboss::FbossError("Invalid BSP Type given to BSP Factory!");
      break;
  }
  std::shared_ptr<Bsp> returnValShared(returnVal);
  return returnValShared;
}

void FanService::kickstart() {
  // Read Config
  pConfig_ = std::make_shared<ServiceConfig>();
  pConfig_->parse();

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
  pControlLogic_ = std::make_shared<ControlLogic>(pConfig_, pBsp_);
}

int FanService::controlFan(/*folly::EventBase* evb*/) {
  int rc = 0;
  uint64_t currentTimeSec = pBsp_->getCurrentTime();
  if (!transitionValueSet_) {
    transitionValueSet_ = true;
    XLOG(INFO)
        << "Upon fan_service start up, program all fan pwm with transitional value of "
        << pConfig_->getPwmTransitionValue();
    pControlLogic_->setTransitionValue();
  }
  // Update Sensor Value based according to fetch frequency
  if ((currentTimeSec - lastSensorFetchTimeSec_) >= getSensorFetchFrequency()) {
    bool sensorReadOK = false;
    bool opticsReadOK = false;

    // Get the updated sensor data
    try {
      pBsp_->getSensorData(pConfig_, pSensorData_);
      sensorReadOK = true;
      XLOG(INFO) << "Successfully fetched sensor data.";
    } catch (std::exception& e) {
      XLOG(ERR) << "Failed to get sensor data with error : " << e.what();
    }

    // Also get the updated optics data
    try {
      // Get the updated optics data
      pBsp_->getOpticsData(pConfig_, pSensorData_);
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

  pBsp_->kickWatchdog(pConfig_);

  return rc;
}

int FanService::runMock(std::string mockInputFile, std::string mockOutputFile) {
  int rc = 0;
  uint64_t timeToAdvanceSec = 0, currentTimeSec = 0;
  bool loopControl = true;
  std::string simulationSensorName;
  float simulationSensorValue;
  // Make sure BSP is a mock bsp type
  if (pConfig_->bspType != fan_config_structs::BspType::kBspMokujin) {
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
      pBsp_->getSensorData(pConfig_, pSensorData_);
      XLOG(INFO) << "Done reading sensor data";
    }
    // Update fan as needed
    if ((currentTimeSec - lastControlExecutionTimeSec_) >=
        getControlFrequency()) {
      lastControlExecutionTimeSec_ = currentTimeSec;
      XLOG(INFO) << "Time for running fan control logic";
      pControlLogic_->updateControl(pSensorData_);
      pBsp_->getOpticsData(pConfig_, pSensorData_);
      XLOG(INFO) << "Done adjusting fan speed";
    }

    // We still need to emulate the watchdog,
    // but kicking it every 5 seconds will fill up the simulation data
    // with all the garbage outputs. So we do it only once per next
    // event calculation (still too much. try not to use watchdog
    // in simulation config, as it will generate too much bogus
    // output)
    pBsp_->kickWatchdog(pConfig_);

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
} // namespace facebook::fboss::platform
