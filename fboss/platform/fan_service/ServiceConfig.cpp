// Copyright 2021- Facebook. All rights reserved.
#include "ServiceConfig.h"
#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h" // @manual=//fboss/lib/platforms:product-info

const std::string TABLE_100G = "speed_100";
const std::string TABLE_200G = "speed_200";
const std::string TABLE_400G = "speed_400";
const std::string TABLE_800G = "speed_800";

namespace facebook::fboss::platform {

ServiceConfig::ServiceConfig() {
  prepareDict();
  jsonConfig_ = "";
  bspType = fan_config_structs::BspType::kBspGeneric;
  pwmBoostValue_ = 0;
  pwmBoostOnDeadFan = 0;
  pwmBoostOnDeadSensor = 0;
  pwmBoostNoQsfpAfterInSec = 0;
  sensorFetchFrequency_ = FSVC_DEFAULT_SENSOR_FETCH_FREQUENCY;
  controlFrequency_ = FSVC_DEFAULT_CONTROL_FREQUENCY;
  pwmLowerThreshold_ = FSVC_DEFAULT_PWM_LOWER_THRES;
  pwmUpperThreshold_ = FSVC_DEFAULT_PWM_UPPER_THRES;
  watchdogEnable_ = false;
}

// This is one of the helper function for parse() method.
// Platform type is detected here, and config string will
// be acquired here.
std::string ServiceConfig::getConfigContents() {
  std::string contents;
  fboss::PlatformMode platformMode;
  XLOG(INFO) << "Detecting the platform type. FRUID File path : "
             << FLAGS_fruid_filepath;
  fboss::PlatformProductInfo productInfo(FLAGS_fruid_filepath);
  productInfo.initialize();
  platformMode = productInfo.getMode();

  XLOG(INFO) << "Trying to fetch the configuration for :  "
             << toString(platformMode);

  // Get the config string of this platform type
  switch (platformMode) {
    case PlatformMode::DARWIN:
      contents = getDarwinFSConfig();
      break;
    case PlatformMode::FAKE_WEDGE:
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
  fan_config_structs::AccessMethod returnVal;
  for (auto& item : values.items()) {
    std::string key = item.first.asString();
    auto value = item.second;
    switch (convertKeywordToIndex(key)) {
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSource:
        if (convertKeywordToIndex(value.asString()) ==
            fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceSysfs) {
          returnVal.accessType() = fan_config_structs::SourceType::kSrcSysfs;
        } else if (
            convertKeywordToIndex(value.asString()) ==
            fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceThrift) {
          returnVal.accessType() = fan_config_structs::SourceType::kSrcThrift;
        } else if (
            convertKeywordToIndex(value.asString()) ==
            fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceUtil) {
          returnVal.accessType() = fan_config_structs::SourceType::kSrcUtil;
        } else if (
            convertKeywordToIndex(value.asString()) ==
            fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceRest) {
          returnVal.accessType() = fan_config_structs::SourceType::kSrcRest;
        } else if (
            convertKeywordToIndex(value.asString()) ==
            fan_config_structs::FsvcConfigDictIndex::
                kFsvcCfgSourceQsfpService) {
          returnVal.accessType() =
              fan_config_structs::SourceType::kSrcQsfpService;
        } else {
          throw facebook::fboss::FbossError(
              "Invalid Access Type : ", value.asString());
        }
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAccessPath:
        returnVal.path() = value.asString();
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

std::vector<std::string> ServiceConfig::splitter(
    std::string str,
    char delimiter) {
  std::vector<std::string> retVal;
  std::stringstream strm(str);
  std::string marker;
  while (std::getline(strm, marker, delimiter)) {
    retVal.push_back(marker);
  }
  return retVal;
}

std::vector<int> ServiceConfig::parseInstance(folly::dynamic value) {
  std::vector<int> returnVal;
  std::string valueStr = value.asString();
  std::vector<std::string> tokens;
  if (valueStr == "all") {
    // Vector of size 0 means all range
    return returnVal;
  }

  tokens = splitter(valueStr, ',');

  for (std::string token : tokens) {
    if (token.find('-') != std::string::npos) {
      // Range
      std::vector<std::string> ranges;
      ranges = splitter(token, '-');
      int begin = std::stoi(ranges[0], nullptr);
      int end = std::stoi(ranges[1], nullptr);
      for (int i = begin; i <= end; i++) {
        returnVal.push_back(i);
      }
    } else {
      // Single value
      int valueInt = std::stoi(token, nullptr);
      returnVal.push_back(valueInt);
    }
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
    switch (convertKeywordToIndex(key)) {
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgRangeLow:
        returnVal.rangeLow = (float)value.asDouble();
        lowSet = true;
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgRangeHigh:
        returnVal.rangeHigh = (float)value.asDouble();
        highSet = true;
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgInvalidRangeAction:
        if (convertKeywordToIndex(value.asString()) ==
            fan_config_structs::FsvcConfigDictIndex::
                kFsvcCfgInvalidRangeActionShutdown)
          returnVal.action = kRangeCheckActionShutdown;
        else if (
            convertKeywordToIndex(value.asString()) ==
            fan_config_structs::FsvcConfigDictIndex::
                kFsvcCfgInvalidRangeActionNone)
          returnVal.action = kRangeCheckActionNone;
        else
          facebook::fboss::FbossError(
              "Invalid Sensor-Out-Of-Range action-type : ", value.asString());
        break;
      case fan_config_structs::FsvcConfigDictIndex::
          kFsvcCfgInvalidRangeTolerance:
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
    switch (convertKeywordToIndex(key)) {
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAlarmMajor:
        returnVal.high_major = (float)value.asDouble();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAlarmMinor:
        returnVal.high_minor = (float)value.asDouble();
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAlarmMinorSoakInSec:
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
        switch (convertKeywordToIndex(key)) {
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgZonesType:
            if (convertKeywordToIndex(value.asString()) ==
                fan_config_structs::FsvcConfigDictIndex::kFsvcCfgTypeMax) {
              newZone.type = fan_config_structs::ZoneType::kZoneMax;
            } else if (
                convertKeywordToIndex(value.asString()) ==
                fan_config_structs::FsvcConfigDictIndex::kFsvcCfgTypeMin) {
              newZone.type = fan_config_structs::ZoneType::kZoneMin;
            } else if (
                convertKeywordToIndex(value.asString()) ==
                fan_config_structs::FsvcConfigDictIndex::kFsvcCfgTypeAvg) {
              newZone.type = fan_config_structs::ZoneType::kZoneAvg;
            } else {
              facebook::fboss::FbossError(
                  "Invalid Zone Type : ", value.asString());
            }
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgZonesFanSlope:
            newZone.slope = (float)value.asDouble();
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensors:
            for (auto& item : value)
              newZone.sensorNames.push_back(item.asString());
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFans:
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
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanPwm:
            newFan.pwm = parseAccessMethod(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanRpm:
            newFan.rpmAccess = parseAccessMethod(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanLed:
            newFan.led = parseAccessMethod(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanGoodLedVal:
            newFan.fanGoodLedVal = static_cast<unsigned>(value.asInt());
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanFailLedVal:
            newFan.fanFailLedVal = static_cast<unsigned>(value.asInt());
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanPresence:
            newFan.presence = parseAccessMethod(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanPresentVal:
            newFan.fanPresentVal = static_cast<unsigned>(value.asInt());
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanMissingVal:
            newFan.fanMissingVal = static_cast<unsigned>(value.asInt());
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmRangeMin:
            newFan.pwmMin = static_cast<unsigned>(value.asInt());
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmRangeMax:
            newFan.pwmMax = static_cast<unsigned>(value.asInt());
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

void ServiceConfig::parseOpticsChapter(folly::dynamic values) {
  try {
    std::string valStr;
    // Go through each optics group
    for (auto& optic : values.items()) {
      Optic newOptic;
      std::vector<std::pair<float, float>> table;
      fan_config_structs::FsvcConfigDictIndex aggregationType;
      // Set name
      std::string opticName = optic.first.asString();
      newOptic.opticName = opticName;
      // Go through each attribute of this optics group
      auto opticAttrib = optic.second;
      for (auto& pair : opticAttrib.items()) {
        auto key = pair.first.asString();
        auto value = pair.second;
        switch (convertKeywordToIndex(key)) {
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAccess:
            newOptic.access = parseAccessMethod(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgInstance:
            newOptic.instanceList = parseInstance(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAggregation:
            aggregationType = convertKeywordToIndex(value.asString());
            if (aggregationType ==
                fan_config_structs::FsvcConfigDictIndex::kFsvcCfgTypeMax) {
              newOptic.aggregation =
                  fan_config_structs::OpticAggregationType::kOpticMax;
            } else {
              XLOG(ERR) << "Invalid Optics data aggregation type : "
                        << value.asString();
              facebook::fboss::FbossError(
                  "Invalid Optics data aggregation type  : ", value.asString());
            }
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed100:
            table = parseTable(value);
            newOptic.tables.push_back(
                {fan_config_structs::OpticTableType::kOpticTable100Generic,
                 table});
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed200:
            table = parseTable(value);
            newOptic.tables.push_back(
                {fan_config_structs::OpticTableType::kOpticTable200Generic,
                 table});
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed400:
            table = parseTable(value);
            newOptic.tables.push_back(
                {fan_config_structs::OpticTableType::kOpticTable400Generic,
                 table});
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed800:
            table = parseTable(value);
            newOptic.tables.push_back(
                {fan_config_structs::OpticTableType::kOpticTable800Generic,
                 table});
            break;
          default:
            XLOG(ERR) << "Invalid Key in Optics Chapter Config : " << key;
            facebook::fboss::FbossError(
                "Invalid Key in Optics Chapter Config : ", key);
            break;
        }
      }
      optics.insert(optics.begin(), newOptic);
    }
  } catch (std::exception& e) {
    XLOG(ERR) << "Config parsing failure during Optics chapter parsing! "
              << e.what();
    throw e;
  }
  return;
}

void ServiceConfig::parseWatchdogChapter(folly::dynamic values) {
  watchdogEnable_ = true;
  for (auto& item : values.items()) {
    auto key = item.first.asString();
    auto value = item.second;
    switch (convertKeywordToIndex(key)) {
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAccess:
        watchdogAccess_ = parseAccessMethod(value);
        break;
      case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgValue:
        watchdogValue_ = value.asString();
        break;
      default:
        XLOG(ERR) << "Invalid Key in Watchdog Chapter Config : " << key;
        facebook::fboss::FbossError(
            "Invalid Key in Watchdog Chapter Config : ", key);
        break;
    }
  }
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
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAccess:
            newSensor.access = parseAccessMethod(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::
              kFsvcCfgSensorAdjustment:
            newSensor.offsetTable = parseTable(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgScale:
            newSensor.scale = static_cast<float>(value.asDouble());
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorType:
            valStr = value.asString();
            if (convertKeywordToIndex(valStr) ==
                fan_config_structs::FsvcConfigDictIndex::
                    kFsvcCfgSensorType4Cuv) {
              newSensor.calculationType = fan_config_structs::
                  SensorPwmCalcType::kSensorPwmCalcFourLinearTable;
            } else if (
                convertKeywordToIndex(valStr) ==
                fan_config_structs::FsvcConfigDictIndex::
                    kFsvcCfgSensorTypeIncrementPid) {
              newSensor.calculationType = fan_config_structs::
                  SensorPwmCalcType::kSensorPwmCalcIncrementPid;
            } else if (
                convertKeywordToIndex(valStr) ==
                fan_config_structs::FsvcConfigDictIndex::
                    kFsvcCfgSensorTypePid) {
              newSensor.calculationType =
                  fan_config_structs::SensorPwmCalcType::kSensorPwmCalcPid;
            } else {
              facebook::fboss::FbossError(
                  "Invalide Sensor PWM Calculation Type ", valStr);
            }
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensor4CuvUp:
            newSensor.fourCurves.normalUp = parseTable(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensor4CuvDown:
            newSensor.fourCurves.normalDown = parseTable(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::
              kFsvcCfgSensor4CuvFailUp:
            newSensor.fourCurves.failUp = parseTable(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::
              kFsvcCfgSensor4CuvFailDown:
            newSensor.fourCurves.failDown = parseTable(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::
              kFsvcCfgSensorIncrpidSetpoint:
            newSensor.incrementPid.setPoint = (float)value.asDouble();
            newSensor.incrementPid.updateMinMaxVal();
            break;
          case fan_config_structs::FsvcConfigDictIndex::
              kFsvcCfgSensorIncrpidPosHyst:
            newSensor.incrementPid.posHysteresis = (float)value.asDouble();
            newSensor.incrementPid.updateMinMaxVal();
            break;
          case fan_config_structs::FsvcConfigDictIndex::
              kFsvcCfgSensorIncrpidNegHyst:
            newSensor.incrementPid.negHysteresis = (float)value.asDouble();
            newSensor.incrementPid.updateMinMaxVal();
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidKd:
            newSensor.incrementPid.kd = (float)value.asDouble();
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidKi:
            newSensor.incrementPid.ki = (float)value.asDouble();
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorIncrpidKp:
            newSensor.incrementPid.kp = (float)value.asDouble();
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorAlarm:
            newSensor.alarm = parseAlarm(value);
            break;
          case fan_config_structs::FsvcConfigDictIndex::kFsvcCfgRangeCheck:
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

opticThresholdTable* FOLLY_NULLABLE ServiceConfig::getConfigOpticTable(
    std::string name,
    fan_config_structs::OpticTableType dataType) {
  for (auto optic = optics.begin(); optic != optics.end(); ++optic) {
    if (optic->opticName == name) {
      for (auto entry = optic->tables.begin(); entry != optic->tables.end();
           ++entry) {
        if (dataType == entry->first) {
          return &entry->second;
        }
      }
    }
  }
  // If not found, return null pointer
  return nullptr;
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

bool ServiceConfig::getWatchdogEnable() {
  return watchdogEnable_;
}
fan_config_structs::AccessMethod ServiceConfig::getWatchdogAccess() {
  return watchdogAccess_;
}
std::string ServiceConfig::getWatchdogValue() {
  return watchdogValue_;
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
  configDict_["name"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgZonesName;
  configDict_["zone_type"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgZonesType;
  configDict_["max"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgTypeMax;
  configDict_["min"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgTypeMin;
  configDict_["avg"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgTypeAvg;
  configDict_["slope"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgZonesFanSlope;
  configDict_["fans"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFans;
  configDict_["pwm"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanPwm;
  configDict_["rpm"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanRpm;
  configDict_["led"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanLed;
  configDict_["fan_good_led_val"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanGoodLedVal;
  configDict_["fan_fail_led_val"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanFailLedVal;
  configDict_["presence"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanPresence;
  configDict_["fan_present_val"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanPresentVal;
  configDict_["fan_missing_val"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgFanMissingVal;
  configDict_["source"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSource;
  configDict_["sysfs"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceSysfs;
  configDict_["util"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceUtil;
  configDict_["thrift"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceThrift;
  configDict_["REST"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceRest;
  configDict_["qsfp_service"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSourceQsfpService;
  configDict_["path"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAccessPath;
  configDict_["sensors"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensors;
  configDict_["adjustment"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorAdjustment;
  configDict_["alarm"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSensorAlarm;
  configDict_["access"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAccess;
  configDict_["alarm_major"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAlarmMajor;
  configDict_["alarm_minor"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAlarmMinor;
  configDict_["alarm_minor_soak"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAlarmMinorSoakInSec;
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
  configDict_["instance"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgInstance;
  configDict_["aggregation"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgAggregation;
  configDict_["speed_100"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed100;
  configDict_["speed_200"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed200;
  configDict_["speed_400"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed400;
  configDict_["speed_800"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgSpeed800;
  configDict_["boost_on_no_qsfp_after"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgNoQsfpBoostInSec;
  configDict_["pwm_range_min"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmRangeMin;
  configDict_["pwm_range_max"] =
      fan_config_structs::FsvcConfigDictIndex::kFsvcCfgPwmRangeMax;
  configDict_["value"] = fan_config_structs::FsvcConfigDictIndex::kFsvcCfgValue;
}
} // namespace facebook::fboss::platform
