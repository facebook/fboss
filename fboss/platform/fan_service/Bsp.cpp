// Copyright 2021- Facebook. All rights reserved.

// Implementation of Bsp class. Refer to .h for functional description
#include "Bsp.h"
#include <string>
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"
// Additional FB helper funtion
#include "common/time/Time.h"

namespace facebook::fboss::platform {

int Bsp::run(const std::string& cmd) {
  int rc = 0;
  folly::Subprocess p({cmd}, folly::Subprocess::Options().pipeStdout());
  auto result = p.communicate();
  rc = p.wait().exitStatus();
  XLOG(INFO) << "Command: " << cmd << " ret:" << rc;
  return rc;
}

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
      case fan_config_structs::SourceType::kSrcQsfpService:
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
    if (pServiceConfig->getShutDownCommand() == "NOT_DEFINED") {
      facebook::fboss::FbossError(
          "Emergency Shutdown Was Called But Not Defined!");
    } else {
      rc = run(pServiceConfig->getShutDownCommand().c_str());
    }
    setEmergencyState(enable);
  }
  return rc;
}

int Bsp::kickWatchdog(std::shared_ptr<ServiceConfig> pServiceConfig) {
  int rc = 0;
  bool sysfsSuccess = false;
  std::string cmdLine;
  if (pServiceConfig->getWatchdogEnable()) {
    auto access = pServiceConfig->getWatchdogAccess();
    switch (*access.accessType_ref()) {
      case fan_config_structs::SourceType::kSrcUtil:
        cmdLine = *access.path_ref() + " " + pServiceConfig->getWatchdogValue();
        rc = run(cmdLine.c_str());
        break;
      case fan_config_structs::SourceType::kSrcSysfs:
        sysfsSuccess = writeSysfs(
            *access.path_ref(), std::stoi(pServiceConfig->getWatchdogValue()));
        rc = sysfsSuccess ? 0 : -1;
        break;
      default:
        throw facebook::fboss::FbossError("Invalid watchdog access type!");
        break;
    }
  }
  return rc;
}

// This method processes one entry in the data array from
// the thrift response, then push the data back to the
// end of opticData array.
void Bsp::processOpticEntries(
    Optic* opticsGroup,
    std::shared_ptr<SensorData> pSensorData,
    uint64_t& currentQsfpSvcTimestamp,
    std::unordered_map<TransceiverID, TransceiverInfo> cacheTable,
    OpticEntry* opticData) {
  std::pair<fan_config_structs::OpticTableType, float> prepData;
  for (auto cacheEntry : cacheTable) {
    int xvrId = static_cast<int>(cacheEntry.first);
    TransceiverInfo& info = cacheEntry.second;
    fan_config_structs::OpticTableType tableType =
        fan_config_structs::OpticTableType::kOpticTableInval;
    // Qsfp_service send the data as double, but fan service use float.
    // So, cast the data to float
    auto sensor = info.sensor_ref();
    // If no sensor available for this transceiver, just skip
    if (!sensor) {
      XLOG(INFO) << "Skipping transceiver " << xvrId
                 << ", as there is no global sensor entry.";
      continue;
    }
    auto qsfpTimestampRef = info.timeCollected_ref();
    if (!qsfpTimestampRef) {
      auto timeStamp = apache::thrift::can_throw(*qsfpTimestampRef);
      if (timeStamp > currentQsfpSvcTimestamp) {
        currentQsfpSvcTimestamp = timeStamp;
      }
    }

    float temp = static_cast<float>(*(sensor->temp_ref()->value_ref()));
    // In the following two cases, do not process the entries and move on
    // 1. temperature from QSFP service is 0.0 - meaning the port is
    //    not populated in qsfp_service or read failure occured. So skip this.
    // 2. Config file specified the port entries we care, but this port
    //    does not belong to the ports we care.
    if (((temp == 0.0)) ||
        ((opticsGroup->instanceList.size() != 0) &&
         (std::find(
              opticsGroup->instanceList.begin(),
              opticsGroup->instanceList.end(),
              xvrId) != opticsGroup->instanceList.end()))) {
      continue;
    }

    // Parse using the definition in qsfp_service/if/transceiver.thrift
    // Detect the speed. If unknown, use the very first table.
    // This field is optional. If missing, we use unknown.
    MediaInterfaceCode mediaInterfaceCode = MediaInterfaceCode::UNKNOWN;
    if (info.moduleMediaInterface_ref()) {
      mediaInterfaceCode = *info.moduleMediaInterface_ref();
    }
    switch (mediaInterfaceCode) {
      case MediaInterfaceCode::UNKNOWN:
        // Use the first table's type for unknown/missing media type
        assert(opticsGroup);
        tableType = opticsGroup->tables[0].first;
        break;
      case MediaInterfaceCode::CWDM4_100G:
      case MediaInterfaceCode::CR4_100G:
      case MediaInterfaceCode::FR1_100G:
        tableType = fan_config_structs::OpticTableType::kOpticTable100Generic;
        break;
      case MediaInterfaceCode::FR4_200G:
        tableType = fan_config_structs::OpticTableType::kOpticTable200Generic;
        break;
      case MediaInterfaceCode::FR4_400G:
      case MediaInterfaceCode::LR4_400G_10KM:
        tableType = fan_config_structs::OpticTableType::kOpticTable400Generic;
        break;
      // No 800G optic yet
      default:
        int intVal = static_cast<int>(mediaInterfaceCode);
        XLOG(ERR) << "Transceiver : " << xvrId
                  << " Unsupported Media Type : " << intVal
                  << "Ignoring this entry";
        break;
    }
    prepData = {tableType, temp};
    opticData->data.push_back(prepData);
  }
}

void Bsp::getOpticsDataThrift(
    Optic* opticsGroup,
    std::shared_ptr<SensorData> pSensorData) {
  bool thriftSuccess = false;
  // Here, we don't really futureGet the transciver data,
  // but use the cached value from the background thread.
  // We use QsfpCache for this.
  std::unordered_map<TransceiverID, TransceiverInfo> cacheTable;
  uint64_t currentQsfpSvcTimestamp = 0;
  try {
    // QsfpCache runs a background thread to do the sync every
    // 30 seconds. The following merely reads the cached data
    // updated in the last sync attempt (thus returns very quickly.)
    cacheTable = qsfpCache_->getAllTransceivers();
    thriftSuccess = true;
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to read optics data from Qsfp for "
              << opticsGroup->opticName;
  }
  // If thrift fails, just return without updating sensor data
  // control logic will see that the timestamp did not change,
  // and do the right error handling.
  if (!thriftSuccess) {
    return;
  }

  // If no entry, create one (unlike sensor entry,
  // optic entiry needs to be created manually,
  // as the data is vector of pairs)
  if (!pSensorData->checkIfOpticEntryExists(opticsGroup->opticName)) {
    std::vector<std::pair<fan_config_structs::OpticTableType, float>> empty;
    pSensorData->setOpticEntry(opticsGroup->opticName, empty, getCurrentTime());
  }
  OpticEntry* opticData = pSensorData->getOpticEntry(opticsGroup->opticName);
  // Clear any old data
  opticData->data.clear();
  // Parse the data
  processOpticEntries(
      opticsGroup, pSensorData, currentQsfpSvcTimestamp, cacheTable, opticData);

  bool dataUpdated = true;
  // Using the timestamp, check if the data is too old or not.
  // QsfpService's cache is updated every 30 seconds. So we check
  // both of the following, to see if this data is old data :
  // 1. qsfpService timestamp is still old
  // 2. fanService timestamp is more than 60 seconds ago
  // If both condition meet, we can infer that we did not get
  // any new data from qsfpService for more than 30 seconds :
  // In this case, we consider the cache data is not meaningful.
  if (currentQsfpSvcTimestamp == opticData->qsfpServiceTimeStamp) {
    uint64_t now = getCurrentTime();
    if (now > opticData->lastOpticsUpdateTimeInSec + 60) {
      dataUpdated = false;
    }
  }

  if (dataUpdated) {
    // Take care of the rest of the meta data in the object
    pSensorData->setLastQsfpSvcTime(getCurrentTime());
    opticData->lastOpticsUpdateTimeInSec = getCurrentTime();
    opticData->qsfpServiceTimeStamp = currentQsfpSvcTimestamp;
    opticData->dataProcessTimeStamp = 0;
    opticData->calculatedPwm = 0;
  } else {
    // After parsing, we realized that this data is same
    // as previous data, according to the timestamp of the update.
    // So we erase all the data, and do not update any meta data
    opticData->data.clear();
  }
}

void Bsp::getOpticsDataSysfs(
    Optic* opticsGroup,
    std::shared_ptr<SensorData> pSensorData) {
  uint64_t nowSec;
  float readVal;
  bool readSuccessful;
  // If we read the data from the sysfs, there is no way
  // to detect the optics type. So we will use the first
  // threshold table we can find (if ever.)
  // Also we return the data as instance 0 (in the case
  // of all) or the first instance in the instance list.
  nowSec = facebook::WallClockUtil::NowInSecFast();
  readSuccessful = false;
  try {
    readVal = getSensorDataSysfs(*opticsGroup->access.path_ref());
    readSuccessful = true;
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to read sysfs " << *opticsGroup->access.path_ref();
  }
  if (readSuccessful) {
    OpticEntry* opticData =
        pSensorData->getOrCreateOpticEntry(opticsGroup->opticName);
    // Use the very first table type to store the data, as we only have data,
    // but without any table type.
    fan_config_structs::OpticTableType firstTableType;
    assert(opticsGroup);
    firstTableType = opticsGroup->tables[0].first;
    std::pair<fan_config_structs::OpticTableType, float> prepData = {
        firstTableType, static_cast<float>(readVal)};
    // Erase any old data, and store the new pair
    opticData->data.clear();
    opticData->data.push_back(prepData);
    opticData->lastOpticsUpdateTimeInSec = getCurrentTime();
    opticData->dataProcessTimeStamp = 0;
    opticData->calculatedPwm = 0;
  }
}

void Bsp::getOpticsData(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> pSensorData) {
  // Only sysfs is read one by one. For other type of read,
  // we set the flags for each type, then read them in batch
  for (auto opticsGroup = pServiceConfig->optics.begin();
       opticsGroup != pServiceConfig->optics.end();
       ++opticsGroup) {
    switch (*opticsGroup->access.accessType_ref()) {
      case fan_config_structs::SourceType::kSrcQsfpService:
      case fan_config_structs::SourceType::kSrcThrift:
        getOpticsDataThrift(&(*opticsGroup), pSensorData);
        break;
      case fan_config_structs::SourceType::kSrcSysfs:
        getOpticsDataSysfs(&(*opticsGroup), pSensorData);
        break;
      case fan_config_structs::SourceType::kSrcRest:
      case fan_config_structs::SourceType::kSrcUtil:
      case fan_config_structs::SourceType::kSrcInvalid:
        throw facebook::fboss::FbossError(
            "Invalid way for fetching optics temperature!");
        break;
    }
  }
  return;
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
  Bsp::createSensorServiceClient(&evb_, sensordThriftPort_)
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
  int retVal = run(charCmd);
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

bool Bsp::initializeQsfpService() {
  qsfpCache_ = std::make_shared<QsfpCache>();
  qsfpCache_->init(&evb_);
  thread_.reset(new std::thread([=] { evb_.loopForever(); }));
  return true;
}

folly::Future<std::unique_ptr<
    facebook::fboss::platform::sensor_service::SensorServiceThriftAsyncClient>>
Bsp::createSensorServiceClient(folly::EventBase* eb, int servicePort) {
  // SR relies on both configerator and smcc being up
  // use raw thrift instead
  auto createClient = [eb, servicePort]() {
    folly::SocketAddress sockAddr("::1", servicePort);
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
