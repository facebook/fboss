// Copyright 2021- Facebook. All rights reserved.
#include "fboss/platform/fan_service/ServiceConfig.h"

#include <folly/json.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <optional>

#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss::platform {

ServiceConfig::ServiceConfig() {
  prepareDict();
  bspType = fan_config_structs::BspType::kBspGeneric;
  pwmBoostValue_ = 0;
  pwmBoostOnDeadFan = 0;
  pwmBoostOnDeadSensor = 0;
  pwmBoostNoQsfpAfterInSec = 0;
  sensorFetchFrequency_ = FSVC_DEFAULT_SENSOR_FETCH_FREQUENCY;
  controlFrequency_ = FSVC_DEFAULT_CONTROL_FREQUENCY;
  pwmLowerThreshold_ = FSVC_DEFAULT_PWM_LOWER_THRES;
  pwmUpperThreshold_ = FSVC_DEFAULT_PWM_UPPER_THRES;
}

// This is one of the helper function for parse() method.
// Platform type is detected here, and config string will
// be acquired here.
std::string ServiceConfig::getConfigContents() {
  std::string contents;
  PlatformType platformType;
  XLOG(INFO) << "Detecting the platform type. FRUID File path : "
             << FLAGS_fruid_filepath;
  fboss::PlatformProductInfo productInfo(FLAGS_fruid_filepath);
  productInfo.initialize();
  platformType = productInfo.getType();

  XLOG(INFO) << "Trying to fetch the configuration for :  "
             << toString(platformType);

  // Get the config string of this platform type
  switch (platformType) {
    case PlatformType::PLATFORM_DARWIN:
      contents = getDarwinFSConfig();
      break;
    case PlatformType::PLATFORM_FAKE_WEDGE:
      contents = getMokujinFSConfig();
      break;
    default:
      throw FbossError(
          "Platform not supported yet : ", productInfo.getProductName());
  }
  XLOG(INFO) << "Finished getting platform config string";
  return contents;
}

// This is another helper function for the main parse() method.
// Config in string form is parsed here.
int ServiceConfig::parseConfigString(std::string contents) {
  // Parse the string contents into json
  auto jsonConfig = folly::parseJson(contents);

  // Translate jsonConfig into the configuration parameters
  // Consider using jsonConfig.keys() or jsonConfig.values()
  // that is, pair=jsonConfig.find(key),
  //          then pair.first is key
  //          pair.second is the value
  //          In this case pair==items().end(), if no key found
  for (auto& pair : jsonConfig.items()) {
    std::string key = pair.first.asString();
    folly::dynamic value = pair.second;
    std::string bspString;
    switch (convertKeywordToIndex(key)) {
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgChapterZones:
        parseZonesChapter(value);
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFans:
        parseFansChapter(value);
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensors:
        parseSensorsChapter(value);
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgOptics:
        parseOpticsChapter(value);
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmBoost:
        pwmBoostValue_ = value.asInt();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmTransition:
        pwmTransitionValue_ = value.asInt();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBoostOnDeadFan:
        pwmBoostOnDeadFan = value.asInt();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBoostOnDeadSensor:
        pwmBoostOnDeadSensor = value.asInt();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgNoQsfpBoostInSec:
        pwmBoostNoQsfpAfterInSec = value.asInt();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmUpper:
        pwmUpperThreshold_ = value.asInt();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmLower:
        pwmLowerThreshold_ = value.asInt();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFscvCfgWatchdogEnable:
        parseWatchdogChapter(value);
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgShutdownCmd:
        shutDownCommand_ = value.asString();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBsp:
        bspString = value.asString();
        parseBspType(bspString);
        break;
      default:
        throw facebook::fboss::FbossError(
            "Unrecognizable fan_service config keyword ", key);
        break;
    }
  }
  return 0;
}

// This is the main entry to get the config for this platform,
// and parse this config.
int ServiceConfig::parse() {
  // Read the raw text from the file
  XLOG(INFO) << "Getting configration string...";
  std::string contents = getConfigContents();
  XLOG(INFO) << "The configuration fetched. Parsing...";
  return parseConfigString(contents);
}

void ServiceConfig::parseBspType(std::string bspString) {
  switch (convertKeywordToIndex(bspString)) {
    case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspGeneric:
      bspType = fan_config_structs::BspType::kBspGeneric;
      break;
    case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspDarwin:
      bspType = fan_config_structs::BspType::kBspDarwin;
      break;
    case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspLassen:
      bspType = fan_config_structs::BspType::kBspLassen;
      break;
    case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspMinipack3:
      bspType = fan_config_structs::BspType::kBspMinipack3;
      break;
    case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspMokujin:
      bspType = fan_config_structs::BspType::kBspMokujin;
      break;
    default:
      throw facebook::fboss::FbossError("Unrecognizable BSP type ", bspString);
      break;
  }
  return;
}

fan_config_structs::AccessMethod ServiceConfig::parseAccessMethod(
    folly::dynamic values) {
  fan_config_structs::AccessMethod accessMethod;
  std::string accessMethodJson = folly::toJson(values);
  apache::thrift::SimpleJSONSerializer::deserialize<
      fan_config_structs::AccessMethod>(accessMethodJson, accessMethod);
  return accessMethod;
}

std::vector<std::pair<float, float>> ServiceConfig::parseTable(
    folly::dynamic value) {
  std::vector<std::pair<float, float>> returnVal;
  // Table : An array of smaller arrays with two elements only
  for (auto& item : value) {
    std::pair<float, float> newPair;
    newPair.first = (float)item[0].asDouble();
    newPair.second = (float)item[1].asDouble();
    returnVal.push_back(newPair);
  }
  return returnVal;
}

fan_config_structs::RangeCheck ServiceConfig::parseRangeCheck(
    folly::dynamic rangeCheckDynamic) {
  fan_config_structs::RangeCheck rangeCheck;
  std::string rangeCheckJson = folly::toJson(rangeCheckDynamic);
  apache::thrift::SimpleJSONSerializer::deserialize<
      fan_config_structs::RangeCheck>(rangeCheckJson, rangeCheck);
  return rangeCheck;
}

fan_config_structs::Alarm ServiceConfig::parseAlarm(
    folly::dynamic alarmDynamic) {
  fan_config_structs::Alarm alarm;
  std::string alarmJson = folly::toJson(alarmDynamic);
  apache::thrift::SimpleJSONSerializer::deserialize<fan_config_structs::Alarm>(
      alarmJson, alarm);
  return alarm;
}

void ServiceConfig::parseZonesChapter(folly::dynamic zonesDynamic) {
  for (const auto& zoneDynamic : zonesDynamic) {
    fan_config_structs::Zone zone;
    std::string zoneJson = folly::toJson(zoneDynamic);
    apache::thrift::SimpleJSONSerializer::deserialize<fan_config_structs::Zone>(
        zoneJson, zone);
    zones.insert(zones.begin(), zone);
  }
}

void ServiceConfig::parseFansChapter(folly::dynamic fansDynamic) {
  for (const auto& fanDynamic : fansDynamic) {
    fan_config_structs::Fan fan;
    std::string fanJson = folly::toJson(fanDynamic);
    apache::thrift::SimpleJSONSerializer::deserialize<fan_config_structs::Fan>(
        fanJson, fan);
    fans.insert(fans.begin(), fan);
  }
}

void ServiceConfig::parseOpticsChapter(folly::dynamic opticsDynamic) {
  for (const auto& opticDynamic : opticsDynamic) {
    fan_config_structs::Optic optic;
    std::string opticJson = folly::toJson(opticDynamic);
    apache::thrift::SimpleJSONSerializer::deserialize<
        fan_config_structs::Optic>(opticJson, optic);
    optics.insert(optics.begin(), optic);
  }
}

void ServiceConfig::parseWatchdogChapter(folly::dynamic watchdogDynamic) {
  fan_config_structs::Watchdog watchdog;
  std::string watchdogJson = folly::toJson(watchdogDynamic);
  apache::thrift::SimpleJSONSerializer::deserialize<
      fan_config_structs::Watchdog>(watchdogJson, watchdog);
  watchdog_ = watchdog;
}

void ServiceConfig::parseSensorsChapter(folly::dynamic sensorsDynamic) {
  for (const auto& sensorDynamic : sensorsDynamic) {
    fan_config_structs::Sensor sensor;
    std::string sensorJson = folly::toJson(sensorDynamic);
    apache::thrift::SimpleJSONSerializer::deserialize<
        fan_config_structs::Sensor>(sensorJson, sensor);
    sensors.insert(sensors.begin(), sensor);
  }
}

float ServiceConfig::getPwmBoostValue() const {
  return pwmBoostValue_;
}

fan_config_structs::FsvcConfigDictIndex ServiceConfig::convertKeywordToIndex(
    std::string keyword) const {
  auto itr = configDict_.find(keyword);
  return itr == configDict_.end()
      ? fan_config_structs::FsvcConfigDictIndex::kFsvcCfgInvalid
      : itr->second;
}

std::optional<fan_config_structs::TempToPwmMap>
ServiceConfig::getConfigOpticTable(
    std::string name,
    fan_config_structs::OpticTableType dataType) {
  for (auto optic = optics.begin(); optic != optics.end(); ++optic) {
    if (*optic->opticName() == name) {
      for (auto entry = optic->tempToPwmMaps()->begin();
           entry != optic->tempToPwmMaps()->end();
           ++entry) {
        if (dataType == entry->first) {
          return entry->second;
        }
      }
    }
  }
  return std::nullopt;
}

std::string ServiceConfig::getShutDownCommand() const {
  return shutDownCommand_;
}

int ServiceConfig::getSensorFetchFrequency() const {
  return sensorFetchFrequency_;
}
int ServiceConfig::getControlFrequency() const {
  return controlFrequency_;
}

int ServiceConfig::getPwmUpperThreshold() const {
  return pwmUpperThreshold_;
}
int ServiceConfig::getPwmLowerThreshold() const {
  return pwmLowerThreshold_;
}

float ServiceConfig::getPwmTransitionValue() const {
  return pwmTransitionValue_;
}

void ServiceConfig::prepareDict() {
  configDict_["bsp"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBsp;
  configDict_["generic"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspGeneric;
  configDict_["darwin"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspDarwin;
  configDict_["mokujin"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspMokujin;
  configDict_["lassen"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspLassen;
  configDict_["sandia"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspSandia;
  configDict_["minipack3"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBspMinipack3;
  configDict_["pwm_boost_value"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmBoost;
  configDict_["pwm_transition_value"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmTransition;
  configDict_["boost_on_dead_fan"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBoostOnDeadFan;
  configDict_["boost_on_dead_sensor"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgBoostOnDeadSensor;
  configDict_["pwm_percent_upper_limit"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmUpper;
  configDict_["pwm_percent_lower_limit"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmLower;
  configDict_["watchdog"] =
      fan_config_structs::FsvcConfigDictIndex::kFscvCfgWatchdogEnable;
  configDict_["shutdown_command"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgShutdownCmd;
  configDict_["zones"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgChapterZones;
  configDict_["fans"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFans;
  configDict_["sensors"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensors;
  configDict_["adjustment"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorAdjustment;
  configDict_["alarm"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorAlarm;
  configDict_["access"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAccess;
  configDict_["type"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorType;
  configDict_["linear_four_curves"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorType4Cuv;
  configDict_["incrementpid"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorTypeIncrementPid;
  configDict_["pid"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorTypePid;
  configDict_["normal_up_table"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensor4CuvUp;
  configDict_["normal_down_table"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensor4CuvDown;
  configDict_["onefail_up_table"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensor4CuvFailUp;
  configDict_["onefail_down_table"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensor4CuvFailDown;
  configDict_["setpoint"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidSetpoint;
  configDict_["positive_hysteresis"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidPosHyst;
  configDict_["negative_hysteresis"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidNegHyst;
  configDict_["kp"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidKp;
  configDict_["ki"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidKi;
  configDict_["kd"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidKd;
  configDict_["range_check"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgRangeCheck;
  configDict_["range_low"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgRangeLow;
  configDict_["range_high"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgRangeHigh;
  configDict_["tolerance"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgInvalidRangeTolerance;
  configDict_["invalid_range_action"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgInvalidRangeAction;
  configDict_["shutdown"] = fan_config_structs::FsvcConfigDictIndex::
      kFsvcCfgInvalidRangeActionShutdown;
  configDict_["no_action"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgInvalidRangeActionNone;
  configDict_["optics"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgOptics;
  configDict_["boost_on_no_qsfp_after"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgNoQsfpBoostInSec;
  configDict_["value"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgValue;
  configDict_["scale"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgScale;
}
} // namespace facebook::fboss::platform
