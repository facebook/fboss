// Copyright 2021- Facebook. All rights reserved.
#include "ServiceConfig.h"

ServiceConfig::ServiceConfig() {
  prepareDict();
  jsonConfig_ = "";
  bspType = kBspGeneric;
  pwmBoostValue_ = 0;
  pwmBoostOnDeadFan = 0;
  pwmBoostOnDeadSensor = 0;
  sensorFetchFrequency_ = FSVC_DEFAULT_SENSOR_FETCH_FREQUENCY;
  controlFrequency_ = FSVC_DEFAULT_CONTROL_FREQUENCY;
  pwmLowerThreshold_ = FSVC_DEFAULT_PWM_LOWER_THRES;
  pwmUpperThreshold_ = FSVC_DEFAULT_PWM_UPPER_THRES;
}

ServiceConfig::~ServiceConfig() {}

bool keyExists_(folly::dynamic dict, std::string key) {
  auto tester = dict.find(key);
  return !(tester == dict.items().end());
}

int ServiceConfig::parse(std::string filename) {
  // Read the raw text from the file
  std::string contents;
  try {
    std::ifstream file;
    file.open(filename.c_str(), std::ios::in);
    if (!file) {
      throw facebook::fboss::FbossError(
          "Unable to open config file ", filename);
    }
    contents = std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
  } catch (std::exception& e) {
    throw facebook::fboss::FbossError(
        "Failed to read config file ", filename, " : ", e.what());
  }

  // Parse the string contents into json
  jsonConfig_ = folly::parseJson(contents);

  // Translate jsonConfig into the configuration parameters
  // Consider using jsonConfig.keys() or jsonConfig.values()
  // that is, pair=jsonConfig.find(key),
  //          then pair.first is key
  //          pair.second is the value
  //          In this case pair==items().end(), if no key found
  for (auto& pair : jsonConfig_.items()) {
    std::string key = pair.first.asString();
    folly::dynamic value = pair.second;
    std::string bspString;
    switch (convertKeywordToIndex(key)) {
      case kFsvcCfgChapterZones:
        parseZonesChapter(value);
        break;
      case kFsvcCfgFans:
        parseFansChapter(value);
        break;
      case kFsvcCfgSensors:
        parseSensorsChapter(value);
        break;
      case kFsvcCfgPwmBoost:
        pwmBoostValue_ = value.asInt();
        break;
      case kFsvcCfgBoostOnDeadFan:
        pwmBoostOnDeadFan = value.asInt();
        break;
      case kFsvcCfgBoostOnDeadSensor:
        pwmBoostOnDeadSensor = value.asInt();
        break;
      case kFsvcCfgPwmUpper:
        pwmUpperThreshold_ = value.asInt();
        break;
      case kFsvcCfgPwmLower:
        pwmLowerThreshold_ = value.asInt();
        break;
      case kFscvCfgWatchdogEnable:
        watchdog_ = value.asBool();
        break;
      case kFsvcCfgShutdownCmd:
        shutDownCommand_ = value.asString();
        break;
      case kFsvcCfgBsp:
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

void ServiceConfig::parseBspType(std::string bspString) {
  switch (convertKeywordToIndex(bspString)) {
    case kFsvcCfgBspGeneric:
      bspType = kBspGeneric;
      break;
    case kFsvcCfgBspDarwin:
      bspType = kBspDarwin;
      break;
    case kFsvcCfgBspLassen:
      bspType = kBspLassen;
      break;
    case kFsvcCfgBspMinipack3:
      bspType = kBspMinipack3;
      break;
    case kFsvcCfgBspMokujin:
      bspType = kBspMokujin;
      break;
    default:
      throw facebook::fboss::FbossError("Unrecognizable BSP type ", bspString);
      break;
  }
  return;
}

AccessMethod ServiceConfig::parseAccessMethod(folly::dynamic values) {
  AccessMethod returnVal;
  for (auto& item : values.items()) {
    std::string key = item.first.asString();
    auto value = item.second;
    switch ((int)convertKeywordToIndex(key)) {
      case kFsvcCfgSource:
        if (convertKeywordToIndex(value.asString()) == kFsvcCfgSourceSysfs)
          returnVal.accessType = kSrcSysfs;
        else if (
            convertKeywordToIndex(value.asString()) == kFsvcCfgSourceThrift)
          returnVal.accessType = kSrcThrift;
        else if (convertKeywordToIndex(value.asString()) == kFsvcCfgSourceUtil)
          returnVal.accessType = kSrcUtil;
        else if (convertKeywordToIndex(value.asString()) == kFsvcCfgSourceRest)
          returnVal.accessType = kSrcRest;
        else
          throw facebook::fboss::FbossError(
              "Invalid Access Type : ", value.asString());
        break;
      case kFsvcCfgAccessPath:
        returnVal.path = value.asString();
        break;
      default:
        XLOG(ERR) << "Invalid Key in Access Method Parsing : " << key;
        throw facebook::fboss::FbossError(
            "Invalid Key in Access Method Parsing : ", key);
        break;
    }
  }
  return returnVal;
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

RangeCheck ServiceConfig::parseRangeCheck(folly::dynamic valueCluster) {
  RangeCheck returnVal;
  returnVal.enabled = true;
  bool lowSet = false;
  bool highSet = false;
  for (auto& item : valueCluster.items()) {
    std::string key = item.first.asString();
    auto value = item.second;
    switch ((int)convertKeywordToIndex(key)) {
      case kFsvcCfgRangeLow:
        returnVal.rangeLow = (float)value.asDouble();
        lowSet = true;
        break;
      case kFsvcCfgRangeHigh:
        returnVal.rangeHigh = (float)value.asDouble();
        highSet = true;
        break;
      case kFsvcCfgInvalidRangeAction:
        if (convertKeywordToIndex(value.asString()) ==
            kFsvcCfgInvalidRangeActionShutdown)
          returnVal.action = kRangeCheckActionShutdown;
        else if (
            convertKeywordToIndex(value.asString()) ==
            kFsvcCfgInvalidRangeActionNone)
          returnVal.action = kRangeCheckActionNone;
        else
          facebook::fboss::FbossError(
              "Invalid Sensor-Out-Of-Range action-type : ", value.asString());
        break;
      case kFsvcCfgInvalidRangeTolerance:
        returnVal.tolerance = value.asInt();
        break;
      default:
        XLOG(ERR) << "Invalid Key in Alarm Parsing : " << key;
        throw facebook::fboss::FbossError(
            "Invalid Key in Alarm Parsing : ", key);
        break;
    }
  }
  if (!(lowSet && highSet)) {
    XLOG(ERR) << "Sensor Range Check Definition is not complete for : ";
    throw facebook::fboss::FbossError(
        "Sensor Range Check Definition is not complete for : ");
  }
  return returnVal;
}

Alarm ServiceConfig::parseAlarm(folly::dynamic valueCluster) {
  Alarm returnVal;
  for (auto& item : valueCluster.items()) {
    std::string key = item.first.asString();
    auto value = item.second;
    switch ((int)convertKeywordToIndex(key)) {
      case kFsvcCfgAlarmMajor:
        returnVal.high_major = (float)value.asDouble();
        break;
      case kFsvcCfgAlarmMinor:
        returnVal.high_minor = (float)value.asDouble();
        break;
      case kFsvcCfgAlarmMinorSoakInSec:
        returnVal.high_minor_soak = value.asInt();
        break;
      default:
        XLOG(ERR) << "Invalid Key in Alarm Parsing : " << key;
        throw facebook::fboss::FbossError(
            "Invalid Key in Alarm Parsing : ", key);
        break;
    }
  }
  return returnVal;
}

void ServiceConfig::parseZonesChapter(folly::dynamic zonesDynamic) {
  try {
    for (auto& zoneItem : zonesDynamic.items()) {
      // Prepare a new Zone entry
      Zone newZone;
      // Supposed to be nested JSON (otherwise, exception is raised)
      // first element should be a zone type
      std::string zoneName = zoneItem.first.asString();
      newZone.zoneName = zoneName;
      // second element should be the nest json with zone attributes
      auto zoneAttrib = zoneItem.second;
      for (auto& pair : zoneAttrib.items()) {
        auto key = pair.first.asString();
        auto value = pair.second;
        switch ((int)convertKeywordToIndex(key)) {
          case kFsvcCfgZonesType:
            if (convertKeywordToIndex(value.asString()) == kFsvcCfgZonesTypeMax)
              newZone.type = kZoneMax;
            else if (
                convertKeywordToIndex(value.asString()) == kFsvcCfgZonesTypeMin)
              newZone.type = kZoneMin;
            else if (
                convertKeywordToIndex(value.asString()) == kFsvcCfgZonesTypeAvg)
              newZone.type = kZoneAvg;
            else
              facebook::fboss::FbossError(
                  "Invalid Zone Type : ", value.asString());
            break;
          case kFsvcCfgZonesFanSlope:
            newZone.slope = (float)value.asDouble();
            break;
          case kFsvcCfgSensors:
            for (auto& item : value)
              newZone.sensorNames.push_back(item.asString());
            break;
          case kFsvcCfgFans:
            for (auto& item : value)
              newZone.fanNames.push_back(item.asString());
            break;
          default:
            XLOG(ERR) << "Invalid Key in Zone Chapter Config : " << key;
            throw facebook::fboss::FbossError(
                "Invalid Key in Zone Chapter Config : ", key);
            break;
        }
      }

      zones.insert(zones.begin(), newZone);
    }

  } catch (std::exception& e) {
    XLOG(ERR) << "Config parsing failure during Zone chapter parsing! "
              << e.what();
    throw e;
  }
  return;
}
void ServiceConfig::parseFansChapter(folly::dynamic value) {
  try {
    for (auto& fan : value.items()) {
      Fan newFan;
      std::string fanName = fan.first.asString();
      newFan.fanName = fanName;
      auto fanAttribs = fan.second;
      for (auto& pair : fanAttribs.items()) {
        auto key = pair.first.asString();
        auto value = pair.second;
        switch (convertKeywordToIndex(key)) {
          case kFsvcCfgFanPwm:
            newFan.pwm = parseAccessMethod(value);
            break;
          case kFsvcCfgFanRpm:
            newFan.rpmAccess = parseAccessMethod(value);
            break;
          default:
            XLOG(ERR) << "Invalid Key in Fan Chapter Config : " << key;
            facebook::fboss::FbossError(
                "Invalid Key in Fan Chapter Config : ", key);
            break;
        }
      }
      fans.insert(fans.begin(), newFan);
    }
  } catch (std::exception& e) {
    XLOG(ERR) << "Config parsing failure during Fans chapter parsing! "
              << e.what();
    throw e;
  }
  return;
}

void ServiceConfig::parseSensorsChapter(folly::dynamic value) {
  try {
    std::string valStr;
    for (auto& sensor : value.items()) {
      Sensor newSensor;
      std::string sensorName = sensor.first.asString();
      newSensor.sensorName = sensorName;
      // second element should be the nest json with zone attributes
      auto sensorAttrib = sensor.second;
      for (auto& pair : sensorAttrib.items()) {
        auto key = pair.first.asString();
        auto value = pair.second;
        switch (convertKeywordToIndex(key)) {
          case kFsvcCfgAccess:
            newSensor.access = parseAccessMethod(value);
            break;
          case kFsvcCfgSensorAdjustment:
            newSensor.offsetTable = parseTable(value);
            break;
          case kFsvcCfgSensorType:
            valStr = value.asString();
            if (convertKeywordToIndex(valStr) == kFsvcCfgSensorType4Cuv)
              newSensor.calculationType = kSensorPwmCalcFourLinearTable;
            else if (
                convertKeywordToIndex(valStr) == kFsvcCfgSensorTypeIncrementPid)
              newSensor.calculationType = kSensorPwmCalcIncrementPid;
            else if (convertKeywordToIndex(valStr) == kFsvcCfgSensorTypePid)
              newSensor.calculationType = kSensorPwmCalcPid;
            else
              facebook::fboss::FbossError(
                  "Invalide Sensor PWM Calculation Type ", valStr);
            break;
          case kFsvcCfgSensor4CuvUp:
            newSensor.fourCurves.normalUp = parseTable(value);
            break;
          case kFsvcCfgSensor4CuvDown:
            newSensor.fourCurves.normalDown = parseTable(value);
            break;
          case kFsvcCfgSensor4CuvFailUp:
            newSensor.fourCurves.failUp = parseTable(value);
            break;
          case kFsvcCfgSensor4CuvFailDown:
            newSensor.fourCurves.failDown = parseTable(value);
            break;
          case kFsvcCfgSensorIncrpidSetpoint:
            newSensor.incrementPid.setPoint = (float)value.asDouble();
            newSensor.incrementPid.updateMinMaxVal();
            break;
          case kFsvcCfgSensorIncrpidPosHyst:
            newSensor.incrementPid.posHysteresis = (float)value.asDouble();
            newSensor.incrementPid.updateMinMaxVal();
            break;
          case kFsvcCfgSensorIncrpidNegHyst:
            newSensor.incrementPid.negHysteresis = (float)value.asDouble();
            newSensor.incrementPid.updateMinMaxVal();
            break;
          case kFsvcCfgSensorIncrpidKd:
            newSensor.incrementPid.kd = (float)value.asDouble();
            break;
          case kFsvcCfgSensorIncrpidKi:
            newSensor.incrementPid.ki = (float)value.asDouble();
            break;
          case kFsvcCfgSensorIncrpidKp:
            newSensor.incrementPid.kp = (float)value.asDouble();
            break;
          case kFsvcCfgSensorAlarm:
            newSensor.alarm = parseAlarm(value);
            break;
          case kFsvcCfgRangeCheck:
            newSensor.rangeCheck = parseRangeCheck(value);
            break;
          default:
            XLOG(ERR) << "Invalid Key in Sensor Chapter Config : " << key;
            facebook::fboss::FbossError(
                "Invalid Key in Sensor Chapter Config : ", key);
            break;
        }
      }
      sensors.insert(sensors.begin(), newSensor);
    }
  } catch (std::exception& e) {
    XLOG(ERR) << "Config parsing failure during Sensor chapter parsing! "
              << e.what();
    throw e;
  }
  return;
}

float ServiceConfig::getPwmBoostValue() {
  return pwmBoostValue_;
}

FsvcConfigDictIndex ServiceConfig::convertKeywordToIndex(std::string keyword) {
  if (configDict_.find(keyword) == configDict_.end()) {
    return kFsvcCfgInvalid;
  }
  return configDict_[keyword];
}

std::string ServiceConfig::getShutDownCommand() {
  return shutDownCommand_;
}

int ServiceConfig::getSensorFetchFrequency() {
  return sensorFetchFrequency_;
}
int ServiceConfig::getControlFrequency() {
  return controlFrequency_;
}

int ServiceConfig::getPwmUpperThreshold() {
  return pwmUpperThreshold_;
}
int ServiceConfig::getPwmLowerThreshold() {
  return pwmLowerThreshold_;
}

void ServiceConfig::prepareDict() {
  configDict_["bsp"] = kFsvcCfgBsp;
  configDict_["generic"] = kFsvcCfgBspGeneric;
  configDict_["darwin"] = kFsvcCfgBspDarwin;
  configDict_["mokujin"] = kFsvcCfgBspMokujin;
  configDict_["lassen"] = kFsvcCfgBspLassen;
  configDict_["minipack3"] = kFsvcCfgBspMinipack3;
  configDict_["pwm_boost_value"] = kFsvcCfgPwmBoost;
  configDict_["boost_on_dead_fan"] = kFsvcCfgBoostOnDeadFan;
  configDict_["boost_on_dead_sensor"] = kFsvcCfgBoostOnDeadSensor;
  configDict_["pwm_percent_upper_limit"] = kFsvcCfgPwmUpper;
  configDict_["pwm_percent_lower_limit"] = kFsvcCfgPwmLower;
  configDict_["watchdog"] = kFscvCfgWatchdogEnable;
  configDict_["shutdown_command"] = kFsvcCfgShutdownCmd;
  configDict_["zones"] = kFsvcCfgChapterZones;
  configDict_["name"] = kFsvcCfgZonesName;
  configDict_["zone_type"] = kFsvcCfgZonesType;
  configDict_["max"] = kFsvcCfgZonesTypeMax;
  configDict_["min"] = kFsvcCfgZonesTypeMin;
  configDict_["avg"] = kFsvcCfgZonesTypeAvg;
  configDict_["slope"] = kFsvcCfgZonesFanSlope;
  configDict_["fans"] = kFsvcCfgFans;
  configDict_["pwm"] = kFsvcCfgFanPwm;
  configDict_["rpm"] = kFsvcCfgFanRpm;
  configDict_["source"] = kFsvcCfgSource;
  configDict_["sysfs"] = kFsvcCfgSourceSysfs;
  configDict_["util"] = kFsvcCfgSourceUtil;
  configDict_["thrift"] = kFsvcCfgSourceThrift;
  configDict_["REST"] = kFsvcCfgSourceRest;
  configDict_["path"] = kFsvcCfgAccessPath;
  configDict_["sensors"] = kFsvcCfgSensors;
  configDict_["adjustment"] = kFsvcCfgSensorAdjustment;
  configDict_["alarm"] = kFsvcCfgSensorAlarm;
  configDict_["access"] = kFsvcCfgAccess;
  configDict_["alarm_major"] = kFsvcCfgAlarmMajor;
  configDict_["alarm_minor"] = kFsvcCfgAlarmMinor;
  configDict_["alarm_minor_soak"] = kFsvcCfgAlarmMinorSoakInSec;
  configDict_["type"] = kFsvcCfgSensorType;
  configDict_["linear_four_curves"] = kFsvcCfgSensorType4Cuv;
  configDict_["incrementpid"] = kFsvcCfgSensorTypeIncrementPid;
  configDict_["pid"] = kFsvcCfgSensorTypePid;
  configDict_["normal_up_table"] = kFsvcCfgSensor4CuvUp;
  configDict_["normal_down_table"] = kFsvcCfgSensor4CuvDown;
  configDict_["onefail_up_table"] = kFsvcCfgSensor4CuvFailUp;
  configDict_["onefail_down_table"] = kFsvcCfgSensor4CuvFailDown;
  configDict_["setpoint"] = kFsvcCfgSensorIncrpidSetpoint;
  configDict_["positive_hysteresis"] = kFsvcCfgSensorIncrpidPosHyst;
  configDict_["negative_hysteresis"] = kFsvcCfgSensorIncrpidNegHyst;
  configDict_["kp"] = kFsvcCfgSensorIncrpidKp;
  configDict_["ki"] = kFsvcCfgSensorIncrpidKi;
  configDict_["kd"] = kFsvcCfgSensorIncrpidKd;
  configDict_["range_check"] = kFsvcCfgRangeCheck;
  configDict_["range_low"] = kFsvcCfgRangeLow;
  configDict_["range_high"] = kFsvcCfgRangeHigh;
  configDict_["tolerance"] = kFsvcCfgInvalidRangeTolerance;
  configDict_["invalid_range_action"] = kFsvcCfgInvalidRangeAction;
  configDict_["shutdown"] = kFsvcCfgInvalidRangeActionShutdown;
  configDict_["no_action"] = kFsvcCfgInvalidRangeActionNone;
}
