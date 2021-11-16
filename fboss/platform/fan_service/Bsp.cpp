// Copyright 2021- Facebook. All rights reserved.

// Implementation of Bsp class. Refer to .h for functional description
#include "Bsp.h"
#include <string>
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"

namespace facebook::fboss::platform {

void Bsp::getSensorData(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> pSensorData) {
  bool fetchOverThrift = false;
  bool fetchOverRest = false;
  bool fetchOverUtil = false;
  // Only sysfs is read one by one. For other type of read,
  // we set the flags for each type, then read them in batch
  for (auto sensor = pServiceConfig->sensors.begin();
       sensor != pServiceConfig->sensors.end();
       ++sensor) {
    uint64_t nowSec;
    float readVal;
    bool readSuccessful;
    switch (sensor->access.accessType) {
      case fan_config_structs::SourceType::kSrcThrift:
        fetchOverThrift = true;
        break;
      case fan_config_structs::SourceType::kSrcRest:
        fetchOverRest = true;
        break;
      case fan_config_structs::SourceType::kSrcUtil:
        fetchOverUtil = true;
        break;
      case fan_config_structs::SourceType::kSrcSysfs:
        nowSec = facebook::WallClockUtil::NowInSecFast();
        readSuccessful = false;
        try {
          readVal = getSensorDataSysfs(sensor->access.path);
          readSuccessful = true;
        } catch (std::exception& e) {
          XLOG(ERR) << "Failed to read sysfs " << sensor->access.path;
        }
        if (readSuccessful)
          pSensorData->updateEntryFloat(sensor->sensorName, readVal, nowSec);
        break;
      case fan_config_structs::SourceType::kSrcInvalid:
      default:
        facebook::fboss::FbossError("Invalid way for fetching sensor data!");
        break;
    }
  }
  // Now, fetch data per different access type othern than sysfs
  // We don't use switch statement, as the config should support
  // mixed read methods. (For example, one sensor is read through thrift.
  // then another sensor is read from sysfs)
  if (fetchOverThrift)
    getSensorDataThrift(pServiceConfig, pSensorData);
  if (fetchOverUtil)
    getSensorDataUtil(pServiceConfig, pSensorData);
  if (fetchOverRest)
    getSensorDataRest(pServiceConfig, pSensorData);

  // Set flag if not set yet
  if (!initialSensorDataRead_)
    initialSensorDataRead_ = true;
}
bool Bsp::checkIfInitialSensorDataRead() const {
  return initialSensorDataRead_;
}

int Bsp::emergencyShutdown(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    bool enable) {
  int rc = 0;
  bool currentState = getEmergencyState();
  if (enable && !currentState) {
    if (pServiceConfig->getShutDownCommand() == "NOT_DEFINED")
      facebook::fboss::FbossError(
          "Emergency Shutdown Was Called But Not Defined!");
    else
      std::system(pServiceConfig->getShutDownCommand().c_str());
    setEmergencyState(enable);
  }
  return rc;
}

uint64_t Bsp::getCurrentTime() const {
  return facebook::WallClockUtil::NowInSecFast();
}

bool Bsp::getEmergencyState() const {
  return emergencyShutdownState_;
}

void Bsp::setEmergencyState(bool state) {
  emergencyShutdownState_ = state;
}

void Bsp::getSensorDataThrift(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> pSensorData) const {
  // Simply call the helper fucntion with empty string vector.
  // (which means we want all sensor data)
  std::vector<std::string> emptyStrVec;
  getSensorDataThriftWithSensorList(pServiceConfig, pSensorData, emptyStrVec);
  return;
}

void Bsp::getSensorDataThriftWithSensorList(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> pSensorData,
    std::vector<std::string> sensorList) const {
  std::string ip = "::1";
  auto params = facebook::servicerouter::ClientParams().setSingleHost(
      ip, sensordThriftPort_);
  auto client =
      facebook::servicerouter::cpp2::getClientFactory()
          .getSRClientUnique<
              facebook::fboss::sensor_service::SensorServiceAsyncClient>(
              "", params);
  auto request = facebook::fboss::sensor_service::SensorReadRequestThrift();
  request.label_ref() = "FanService BSP Request";
  request.optionalSensorList_ref() = sensorList;
  auto response =
      folly::coro::blockingWait(client->co_getSensorValues(request));
  auto responseSensorData = response.sensorData_ref();
  for (auto& it : *responseSensorData) {
    std::string key = *it.name_ref();
    float value = *it.value_ref();
    int64_t timeStamp = *it.timeStamp_ref();
    // uint64_t timeStamp=facebook::WallClockUtil::NowInSecFast();
    pSensorData->updateEntryFloat(key, value, timeStamp);
  }
}

void Bsp::getSensorDataRest(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> /*pSensorData*/) const {
  facebook::fboss::FbossError("getSensorDataRest is NOT IMPLEMENTED YET!");
}

void Bsp::getSensorDataUtil(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> /*pSensorData*/) const {
  facebook::fboss::FbossError("getSensorDataUtil is NOT IMPLEMENTED YET!");
}

// Sysfs may fail, but fan_service should keep running even
// after these failures. Therefore, in case of failure,
// we just throw exception and let caller handle it.
float Bsp::getSensorDataSysfs(std::string path) const {
  return readSysfs(path);
}

float Bsp::readSysfs(std::string path) const {
  float retVal;
  std::ifstream juicejuice(path);
  std::string buf;
  try {
    std::getline(juicejuice, buf);
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to read sysfs path " << path;
    throw e;
  }
  try {
    retVal = std::stof(buf);
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to convert sysfs read to float!! ";
    throw e;
  }
  return retVal;
}

bool Bsp::setFanPwmSysfs(std::string path, int pwm) {
  std::string pwmStr = std::to_string(pwm);
  bool success = true;
  try {
    std::ofstream out(path);
    out << pwmStr;
    out.close();
  } catch (std::exception& e) {
    success = false;
  }
  return success;
}

std::string Bsp::replaceAllString(
    std::string original,
    std::string src,
    std::string tgt) const {
  std::string retVal = original;
  size_t index = 0;
  index = retVal.find(src, index);
  while (index != std::string::npos) {
    retVal.replace(index, src.size(), tgt);
    index = retVal.find(src, index);
  }
  return retVal;
}

bool Bsp::setFanPwmShell(std::string command, std::string fanName, int pwm) {
  std::string pwmStr = std::to_string(pwm);
  command = replaceAllString(command, "_NAME_", fanName);
  command = replaceAllString(command, "_PWM_", pwmStr);
  const char* charCmd = command.c_str();
  int retVal = system(charCmd);
  // Return if this command execution was successful
  return (retVal == 0);
}

} // namespace facebook::fboss::platform
