// Copyright 2021- Facebook. All rights reserved.

// Implementation of Bsp class. Refer to .h for functional description
#include "Bsp.h"
#include <string>
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"
// Additional FB helper funtion
#include "common/time/Time.h"

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
    switch (*sensor->access.accessType_ref()) {
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
          readVal = getSensorDataSysfs(*sensor->access.path_ref());
          readSuccessful = true;
        } catch (std::exception& e) {
          XLOG(ERR) << "Failed to read sysfs " << *sensor->access.path_ref();
        }
        if (readSuccessful) {
          pSensorData->updateEntryFloat(sensor->sensorName, readVal, nowSec);
        }
        break;
      case fan_config_structs::SourceType::kSrcInvalid:
      default:
        throw facebook::fboss::FbossError(
            "Invalid way for fetching sensor data!");
        break;
    }
  }
  // Now, fetch data per different access type other than sysfs
  // We don't use switch statement, as the config should support
  // mixed read methods. (For example, one sensor is read through thrift.
  // then another sensor is read from sysfs)
  if (fetchOverThrift) {
    getSensorDataThrift(pServiceConfig, pSensorData);
  }
  if (fetchOverUtil) {
    getSensorDataUtil(pServiceConfig, pSensorData);
  }
  if (fetchOverRest) {
    getSensorDataRest(pServiceConfig, pSensorData);
  }

  // Set flag if not set yet
  if (!initialSensorDataRead_) {
    initialSensorDataRead_ = true;
  }
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
    std::shared_ptr<SensorData> pSensorData) {
  // Simply call the helper fucntion with empty string vector.
  // (which means we want all sensor data)
  std::vector<std::string> emptyStrVec;
  getSensorDataThriftWithSensorList(pServiceConfig, pSensorData, emptyStrVec);
  return;
}

void Bsp::getSensorDataThriftWithSensorList(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> pSensorData,
    std::vector<std::string> sensorList) {
  Bsp::createSensorServiceClient(&evb_)
      .thenValue([sensorList](auto&& client) {
        // use empty list to fetch all transceivers
        std::vector<int32_t> ids;
        auto options = Bsp::getRpcOptions();
        return client->future_getSensorValuesByNames(options, sensorList);
      })
      .wait()
      .thenValue([pSensorData](auto&& response) mutable {
        auto responseSensorData = response.sensorData_ref();
        for (auto& it : *responseSensorData) {
          std::string key = *it.name_ref();
          float value = *it.value_ref();
          int64_t timeStamp = *it.timeStamp_ref();
          pSensorData->updateEntryFloat(key, value, timeStamp);
          XLOG(INFO) << "Storing sensor " << key << " with value " << value
                     << " timestamp : " << timeStamp;
        }
        XLOG(INFO) << "Got sensor data from sensor_service";
      })
      .thenError(folly::tag_t<std::exception>{}, [](const std::exception& e) {
        XLOG(ERR) << "Exception talking to sensor_service" << e.what();
      });
  return;
}

void Bsp::getSensorDataRest(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> /*pSensorData*/) {
  throw facebook::fboss::FbossError(
      "getSensorDataRest is NOT IMPLEMENTED YET!");
}

void Bsp::getSensorDataUtil(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> /*pSensorData*/) {
  throw facebook::fboss::FbossError(
      "getSensorDataUtil is NOT IMPLEMENTED YET!");
}

// Sysfs may fail, but fan_service should keep running even
// after these failures. Therefore, in case of failure,
// we just throw exception and let caller handle it.
float Bsp::getSensorDataSysfs(std::string path) {
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

bool Bsp::writeSysfs(std::string path, int value) {
  std::string valueStr = std::to_string(value);
  bool success = true;
  try {
    std::ofstream out(path);
    out << valueStr;
    out.close();
  } catch (std::exception& e) {
    success = false;
  }
  return success;
}

bool Bsp::setFanPwmSysfs(std::string path, int pwm) {
  // Run the common sysfs access function
  return writeSysfs(path, pwm);
}

bool Bsp::setFanLedSysfs(std::string path, int pwm) {
  // Run the common sysfs access function
  return writeSysfs(path, pwm);
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

bool Bsp::setFanShell(
    std::string command,
    std::string keySymbol,
    std::string fanName,
    int pwm) {
  std::string pwmStr = std::to_string(pwm);
  command = replaceAllString(command, "_NAME_", fanName);
  // keySymbol here is what we will replace with the actual value
  // (from the fan_service config file.)
  command = replaceAllString(command, keySymbol, pwmStr);
  const char* charCmd = command.c_str();
  int retVal = system(charCmd);
  // Return if this command execution was successful
  return (retVal == 0);
}

bool Bsp::setFanPwmShell(std::string command, std::string fanName, int pwm) {
  // Call the common function with _PWM_ as the token to replace
  return setFanShell(command, "_PWM_", fanName, pwm);
}

bool Bsp::setFanLedShell(std::string command, std::string fanName, int value) {
  // Call the common function with _VALUE_ as the token to replace
  return setFanShell(command, "_VALUE_", fanName, value);
}

folly::Future<std::unique_ptr<
    facebook::fboss::platform::sensor_service::SensorServiceThriftAsyncClient>>
Bsp::createSensorServiceClient(folly::EventBase* eb) {
  // SR relies on both configerator and smcc being up
  // use raw thrift instead
  auto createClient = [eb]() {
    folly::SocketAddress sockAddr("::1", 5910);
    // secure client
    auto certKeyPair =
        facebook::security::CertPathPicker::getClientCredentialPaths(true);
    if (certKeyPair.first.empty() or certKeyPair.second.empty()) {
      LOG(INFO) << "empty cert or key => cert: " << certKeyPair.first
                << ", key: " << certKeyPair.second
                << " Creating plain text client.";
      auto socket =
          folly::AsyncSocket::newSocket(eb, sockAddr, kSensorConnTimeoutMs);
      socket->setSendTimeout(kSensorSendTimeoutMs);
      auto channel =
          apache::thrift::HeaderClientChannel::newChannel(std::move(socket));
      return std::make_unique<sensor_service::SensorServiceThriftAsyncClient>(
          std::move(channel));
    } else {
      auto ctx = std::make_shared<folly::SSLContext>();
      ctx->loadCertificate(certKeyPair.first.c_str());
      ctx->loadPrivateKey(certKeyPair.second.c_str());

      auto socket =
          folly::AsyncSSLSocket::UniquePtr(new folly::AsyncSSLSocket(ctx, eb));
      socket->setSendTimeout(kSensorSendTimeoutMs);
      socket->connect(nullptr, sockAddr, kSensorConnTimeoutMs);
      auto channel =
          apache::thrift::HeaderClientChannel::newChannel(std::move(socket));
      return std::make_unique<sensor_service::SensorServiceThriftAsyncClient>(
          std::move(channel));
    }
  };
  return folly::via(eb, createClient);
}

apache::thrift::RpcOptions Bsp::getRpcOptions() {
  apache::thrift::RpcOptions opts;
  opts.setTimeout(std::chrono::milliseconds(kSensorSendTimeoutMs));
  return opts;
}

Bsp::~Bsp() {
  if (thread_) {
    evb_.runInEventBaseThread([this] { evb_.terminateLoopSoon(); });
    thread_->join();
  }
}
} // namespace facebook::fboss::platform
