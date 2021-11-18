// Copyright 2021- Facebook. All rights reserved.

// Implementation of FanService class. Refer to .h file
// for functional description
#include "fboss/platform/fan_service/FanService.h"

#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"

namespace facebook::fboss::platform {
FanService::FanService(std::string configFileName) {
  lastControlExecutionTimeSec_ = 0;
  lastSensorFetchTimeSec_ = 0;

  pBsp_ = NULL;
  pSensorData_ = NULL;

  odsTier_ = "";

  cfgFile_ = configFileName;
  enableOdsStreamer_ = true;

  return;
}

bool FanService::getOdsStreamerEnable() {
  return enableOdsStreamer_;
}

void FanService::setOdsStreamerEnable(bool enable) {
  enableOdsStreamer_ = enable;
}

void FanService::setControlFrequency(uint64_t sec) {
  controlFrequencySec_ = sec;
}

unsigned int FanService::getControlFrequency() {
  return controlFrequencySec_;
}

void FanService::setSensorFetchFrequency(uint64_t sec) {
  sensorFetchFrequencySec_ = sec;
}

unsigned int FanService::getSensorFetchFrequency() {
  return sensorFetchFrequencySec_;
}

void FanService::setOdsTier(std::string oT) {
  odsTier_ = oT;
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
  int rc = 0;

  // Read Config
  pConfig_ = std::make_shared<ServiceConfig>();
  pConfig_->parse(cfgFile_);

  // Get the proper BSP object from BSP factory,
  // according to the parsed config, then run init routine.
  pBsp_ = BspFactory();
  // Initialize SensorData
  pSensorData_ = std::make_shared<SensorData>();

  // Start ODS Streamer and attach sensors
  setOdsStreamerEnable(true);
  pOdsStreamer_ = std::make_shared<OdsStreamer>(odsTier_);

  // Start control logic, and attach bsp and sensors
  pControlLogic_ = std::make_shared<ControlLogic>(pConfig_, pBsp_);
}

int FanService::controlFan(/*folly::EventBase* evb*/) {
  int rc = 0;
  bool hasSensorDataUpdate = false;
  uint64_t currentTimeSec = facebook::WallClockUtil::NowInSecFast();
  // Update Sensor Value based according to fetch frequency
  if ((currentTimeSec - lastSensorFetchTimeSec_) >=
      pConfig_->getSensorFetchFrequency()) {
    lastSensorFetchTimeSec_ = currentTimeSec;
    // Get the updated sensor data
    pBsp_->getSensorData(pConfig_, pSensorData_);
    hasSensorDataUpdate = true;
  }
  // Change Fan PWM as needed according to control execution frequency
  if ((currentTimeSec - lastControlExecutionTimeSec_) >=
      pConfig_->getControlFrequency()) {
    lastControlExecutionTimeSec_ = currentTimeSec;
    pControlLogic_->updateControl(pSensorData_);
  }

  return rc;
}

void FanService::publishToOds(folly::EventBase* evb) {
  pOdsStreamer_->postData(evb, *pSensorData_);
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
  int sensorIntervalSec = (int)(pConfig_->getSensorFetchFrequency());
  int fanIntervalSec = (int)(pConfig_->getControlFrequency());

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
    if ((currentTimeSec - lastSensorFetchTimeSec_) >=
        pConfig_->getSensorFetchFrequency()) {
      lastSensorFetchTimeSec_ = currentTimeSec;
      XLOG(INFO) << "Time to read sensor data";
      pBsp_->getSensorData(pConfig_, pSensorData_);
      XLOG(INFO) << "Done reading sensor data";
    }
    // Update fan as needed
    if ((currentTimeSec - lastControlExecutionTimeSec_) >=
        pConfig_->getControlFrequency()) {
      lastControlExecutionTimeSec_ = currentTimeSec;
      XLOG(INFO) << "Time for running fan control logic";
      pControlLogic_->updateControl(pSensorData_);
      XLOG(INFO) << "Done adjusting fan speed";
    }

    // Figure out when is the next event,
    // so that we can jump to that time in the future.
    timeToAdvanceSec =
        lastSensorFetchTimeSec_ + pConfig_->getSensorFetchFrequency();
    if (timeToAdvanceSec >
        lastControlExecutionTimeSec_ + pConfig_->getControlFrequency()) {
      timeToAdvanceSec =
          lastControlExecutionTimeSec_ + pConfig_->getControlFrequency();
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
