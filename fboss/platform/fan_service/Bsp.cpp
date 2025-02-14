// Copyright 2021- Facebook. All rights reserved.

#include "fboss/platform/fan_service/Bsp.h"

#include <fstream>
#include <string>

#include <folly/logging/xlog.h>

#include "common/time/Time.h"
#include "fboss/agent/FbossError.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/platform/fan_service/DataFetcher.h"
#include "fboss/platform/fan_service/FsdbSensorSubscriber.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"
#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"

using namespace folly::literals::shell_literals;
namespace constants =
    facebook::fboss::platform::fan_service::fan_service_config_constants;

DEFINE_bool(
    subscribe_to_qsfp_data_from_fsdb,
    false,
    "For subscribing to qsfp state and stats from FSDB");

namespace facebook::fboss::platform::fan_service {

Bsp::Bsp(const FanServiceConfig& config) : config_(config) {
  fsdbPubSubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>("fan_service");
  fsdbSensorSubscriber_ =
      std::make_unique<FsdbSensorSubscriber>(fsdbPubSubMgr_.get());
  if (FLAGS_subscribe_to_stats_from_fsdb) {
    fsdbSensorSubscriber_->subscribeToSensorServiceStat();
    if (FLAGS_subscribe_to_qsfp_data_from_fsdb) {
      fsdbSensorSubscriber_->subscribeToQsfpServiceStat();
      fsdbSensorSubscriber_->subscribeToQsfpServiceState();
    }
  }
  thread_.reset(new std::thread([=, this] { evbSensor_.loopForever(); }));
}

void Bsp::getSensorData(std::shared_ptr<SensorData> pSensorData) {
  bool fetchOverThrift = false;
  bool fetchFromFsdb = false;

  for (const auto& sensor : *config_.sensors()) {
    auto sensorAccessType = *sensor.access()->accessType();
    if (sensorAccessType == constants::ACCESS_TYPE_THRIFT()) {
      if (FLAGS_subscribe_to_stats_from_fsdb) {
        fetchFromFsdb = true;
      } else {
        fetchOverThrift = true;
      }
    } else {
      throw facebook::fboss::FbossError(
          "Invalid way for fetching sensor data!");
    }
  }

  if (fetchOverThrift) {
    getSensorDataThrift(pSensorData);
  }

  if (fetchFromFsdb) {
    auto subscribedData = fsdbSensorSubscriber_->getSensorData();
    for (const auto& [sensorName, sensorData] : subscribedData) {
      // Skip adding an entry for this sensor if either the value or timestamp
      // fields are not set
      if (sensorData.value().has_value() &&
          sensorData.timeStamp().has_value()) {
        pSensorData->updateSensorEntry(
            *sensorData.name(), *sensorData.value(), *sensorData.timeStamp());
      }
    }
    XLOG(INFO) << fmt::format(
        "Got sensor data from fsdb.  Item count: {}", subscribedData.size());
  }

  if (!initialSensorDataRead_) {
    initialSensorDataRead_ = true;
  }
}
bool Bsp::checkIfInitialSensorDataRead() const {
  return initialSensorDataRead_;
}

int Bsp::emergencyShutdown(bool enable) {
  int rc = 0;
  bool currentState = getEmergencyState();
  if (enable && !currentState) {
    if (*config_.shutdownCmd() == "NOT_DEFINED") {
      facebook::fboss::FbossError(
          "Emergency Shutdown Was Called But Not Defined!");
    } else {
      auto [exitStatus, standardOut] =
          PlatformUtils().execCommand(*config_.shutdownCmd());
      rc = exitStatus;
    }
    setEmergencyState(enable);
  }
  return rc;
}

void Bsp::kickWatchdog() {
  if (!config_.watchdog()) {
    return;
  }
  std::string valueStr = std::to_string(*config_.watchdog()->value());
  if (!writeToWatchdog(valueStr)) {
    XLOG(ERR) << "Failed to kick watchdog";
  }
}

void Bsp::closeWatchdog() {
  if (!watchdogFd_.has_value()) {
    return;
  }
  std::cout << "Closing watchdog" << std::endl;
  try {
    writeToWatchdog("V");
    close(watchdogFd_.value());
  } catch (std::exception& e) {
    XLOG(ERR) << "Error closing watchdog: " << e.what();
  }
}

bool Bsp::writeToWatchdog(const std::string& value) {
  std::string cmdLine;
  if (!config_.watchdog().has_value()) {
    return false;
  }
  try {
    auto sysfsPath = config_.watchdog()->sysfsPath()->c_str();
    if (!watchdogFd_.has_value()) {
      watchdogFd_ = open(sysfsPath, O_WRONLY);
    }
    return writeFd(watchdogFd_.value(), value);
  } catch (std::exception& e) {
    XLOG(ERR) << "Could not write to watchdog: " << e.what();
    return false;
  }
}

std::vector<std::pair<std::string, float>> Bsp::processOpticEntries(
    const Optic& opticsGroup,
    std::shared_ptr<SensorData> pSensorData,
    uint64_t& currentQsfpSvcTimestamp,
    const std::map<int32_t, TransceiverInfo>& transceiverInfoMap) {
  std::vector<std::pair<std::string, float>> data{};
  for (const auto& [xvrId, transceiverInfo] : transceiverInfoMap) {
    const TcvrState& tcvrState = *transceiverInfo.tcvrState();
    const TcvrStats& tcvrStats = *transceiverInfo.tcvrStats();

    if (!tcvrStats.sensor()) {
      XLOG(ERR) << fmt::format(
          "Transceiver id {} has no sensor data. Ignoring.", xvrId);
      continue;
    }

    if (*tcvrStats.timeCollected() > currentQsfpSvcTimestamp) {
      currentQsfpSvcTimestamp = *tcvrStats.timeCollected();
    }

    float temp = static_cast<float>(*(tcvrStats.sensor()->temp()->value()));
    // In the following two cases, do not process the entries and move on
    // 1. temperature from QSFP service is 0.0 - meaning the port is
    //    not populated in qsfp_service or read failure occured. So skip this.
    // 2. Config file specified the port entries we care, but this port
    //    does not belong to the ports we care.
    if (((temp == 0.0)) ||
        ((!opticsGroup.portList()->empty()) &&
         (std::find(
              opticsGroup.portList()->begin(),
              opticsGroup.portList()->end(),
              xvrId) != opticsGroup.portList()->end()))) {
      continue;
    }

    auto mediaInterfaceCode = tcvrState.moduleMediaInterface()
        ? *tcvrState.moduleMediaInterface()
        : MediaInterfaceCode::UNKNOWN;

    // Parse using the definition in qsfp_service/if/transceiver.thrift
    std::string opticType{};
    switch (mediaInterfaceCode) {
      case MediaInterfaceCode::UNKNOWN:
        // Use the first table's type for unknown/missing media type
        opticType = opticsGroup.tempToPwmMaps()->begin()->first;
        break;
      case MediaInterfaceCode::CWDM4_100G:
      case MediaInterfaceCode::CR4_100G:
      case MediaInterfaceCode::FR1_100G:
        opticType = constants::OPTIC_TYPE_100_GENERIC();
        break;
      case MediaInterfaceCode::FR4_200G:
        opticType = constants::OPTIC_TYPE_200_GENERIC();
        break;
      case MediaInterfaceCode::FR4_400G:
      case MediaInterfaceCode::LR4_400G_10KM:
      case MediaInterfaceCode::DR4_400G:
        opticType = constants::OPTIC_TYPE_400_GENERIC();
        break;
      case MediaInterfaceCode::FR4_2x400G:
      case MediaInterfaceCode::FR4_LITE_2x400G:
      case MediaInterfaceCode::DR4_2x400G:
      case MediaInterfaceCode::FR8_800G:
        opticType = constants::OPTIC_TYPE_800_GENERIC();
        break;
      default:
        XLOG(INFO) << fmt::format(
            "Transceiver id {} has unsupported media type {}. Ignoring.",
            xvrId,
            int(mediaInterfaceCode));
        break;
    }
    data.emplace_back(opticType, temp);
  }
  return data;
}

void Bsp::getOpticsDataFromQsfpSvc(
    const Optic& opticsGroup,
    std::shared_ptr<SensorData> pSensorData) {
  std::map<int32_t, TransceiverInfo> transceiverInfoMap{};
  uint64_t currentQsfpSvcTimestamp = 0;
  try {
    if (FLAGS_subscribe_to_stats_from_fsdb &&
        FLAGS_subscribe_to_qsfp_data_from_fsdb) {
      auto subscribedQsfpDataState = fsdbSensorSubscriber_->getTcvrState();
      auto subscribedQsfpDataStats = fsdbSensorSubscriber_->getTcvrStats();
      for (const auto& [tcvrId, tcvrState] : subscribedQsfpDataState) {
        auto tcvrStatIt = subscribedQsfpDataStats.find(tcvrId);
        // Expect to see a stat if state exists. If not, ignore this port
        if (tcvrStatIt == subscribedQsfpDataStats.end()) {
          continue;
        }
        TransceiverInfo transceiverInfo{};
        transceiverInfo.tcvrState() = tcvrState;
        transceiverInfo.tcvrStats() = tcvrStatIt->second;
        transceiverInfoMap.emplace(tcvrId, transceiverInfo);
      }
    } else {
      getTransceivers(transceiverInfoMap, evb_);
    }
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to read optics data from Qsfp for "
              << *opticsGroup.opticName()
              << ", exception: " << folly::exceptionStr(e);
    // If thrift fails, just return without updating sensor data
    // control logic will see that the timestamp did not change,
    // and do the right error handling.
    return;
  }

  // Parse the data
  auto data = processOpticEntries(
      opticsGroup, pSensorData, currentQsfpSvcTimestamp, transceiverInfoMap);

  auto opticEntry = pSensorData->getOpticEntry(*opticsGroup.opticName());
  // Using the timestamp, check if the data is too old or not.
  // QsfpService's cache is updated every 30 seconds. So we check
  // both of the following, to see if this data is old data :
  // 1. qsfpService timestamp is still old
  // 2. fanService timestamp is more than 60 seconds ago
  // If both condition meet, we can infer that we did not get
  // any new data from qsfpService for more than 30 seconds :
  // In this case, we consider the cache data is not meaningful.
  uint64_t now = getCurrentTime();
  if (currentQsfpSvcTimestamp == opticEntry->qsfpServiceTimeStamp &&
      now > opticEntry->lastOpticsUpdateTimeInSec + 60) {
    pSensorData->resetOpticData(*opticsGroup.opticName());
  } else {
    pSensorData->updateOpticEntry(
        *opticsGroup.opticName(), data, currentQsfpSvcTimestamp);
  }
  XLOG(INFO) << fmt::format(
      "Got optics data from Qsfp. Data Size: {}. QsfpSvcTimestamp: {}",
      data.size(),
      currentQsfpSvcTimestamp);
}

void Bsp::getOpticsData(std::shared_ptr<SensorData> pSensorData) {
  for (const auto& optic : *config_.optics()) {
    auto accessType = *optic.access()->accessType();
    if (accessType == constants::ACCESS_TYPE_QSFP() ||
        accessType == constants::ACCESS_TYPE_THRIFT()) {
      getOpticsDataFromQsfpSvc(optic, pSensorData);
    } else {
      throw facebook::fboss::FbossError(
          "Invalid way for fetching optics temperature!");
    }
  }
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

void Bsp::getSensorDataThrift(std::shared_ptr<SensorData> pSensorData) {
  auto sensorReadResponse =
      getSensorValueThroughThrift(sensordThriftPort_, evbSensor_);
  for (auto& sensorData : *sensorReadResponse.sensorData()) {
    // Value and Timestamp are not set for failed sensors. Skip them
    if (sensorData.value() && sensorData.timeStamp()) {
      pSensorData->updateSensorEntry(
          *sensorData.name(), *sensorData.value(), *sensorData.timeStamp());
      XLOG(DBG1) << fmt::format(
          "Storing sensor {} with value {} timestamp {}",
          *sensorData.name(),
          *sensorData.value(),
          *sensorData.timeStamp());
    }
  }
  XLOG(INFO) << fmt::format(
      "Got sensor data from sensor_service.  Item count: {}",
      sensorReadResponse.sensorData()->size());
}

float Bsp::readSysfs(const std::string& path) const {
  float retVal;
  std::ifstream juicejuice(path);
  std::string buf = facebook::fboss::readSysfs(path);
  try {
    retVal = std::stof(buf);
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to convert sysfs read to float!! ";
    throw e;
  }
  return retVal;
}

bool Bsp::writeSysfs(const std::string& path, int value) {
  std::string valueStr = std::to_string(value);
  return facebook::fboss::writeSysfs(path, valueStr);
}

bool Bsp::setFanPwmSysfs(const std::string& path, int pwm) {
  return writeSysfs(path, pwm);
}

bool Bsp::setFanLedSysfs(const std::string& path, int pwm) {
  return writeSysfs(path, pwm);
}

Bsp::~Bsp() {
  if (thread_) {
    evbSensor_.runInEventBaseThread([this] { evbSensor_.terminateLoopSoon(); });
    thread_->join();
  }
  fsdbSensorSubscriber_.reset();
  fsdbPubSubMgr_.reset();
  closeWatchdog();
}

} // namespace facebook::fboss::platform::fan_service
