// Copyright 2021- Facebook. All rights reserved.

// Implementation of Mokujin class. Refer to .h file
// for functional description.
#include "Mokujin.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform {

Mokujin::Mokujin(const fan_config_structs::FanServiceConfig& config)
    : Bsp(config) {}

void Mokujin::openIOFiles(std::string iFileName, std::string oFileName) {
  try {
    iFs_.open(iFileName, std::ifstream::in);
    iFOpen_ = true;
    oFs_.open(oFileName, std::ofstream::out);
    oFOpen_ = true;
  } catch (std::exception& e) {
    if (!iFOpen_) {
      XLOG(ERR) << "Failed to open input file : " << iFileName;
    } else {
      XLOG(ERR) << "Failed to open output file : " << oFileName;
    }
  }
  // If inputFile is open, do a dummy read to fill up the next event buffer.
  if (iFOpen_) {
    nextEventSensor_ = "dummy";
    nextEventValue_ = 0.0;
    nextEventTimeSec_ = 0;
    std::string dummyS;
    float dummyF;
    getNextSensorEvent(0, dummyS, dummyF);
  }
  return;
}
Mokujin::~Mokujin() {
  closeFiles();
}

void Mokujin::closeFiles() {
  // Gracefully close the input file and the output file.
  try {
    iFs_.close();
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to close the input file : " << e.what();
  }
  try {
    oFs_.close();
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to close the output file : " << e.what();
  }
  return;
}

void Mokujin::setTimeStamp(uint64_t stp) {
  currentTimeStampSec_ = stp;
}

uint64_t Mokujin::getTimeStamp() {
  return currentTimeStampSec_;
}

void Mokujin::updateSimulationData(std::string key, float value) {
  simulatedSensorRead_[key] = value;
}
void Mokujin::getSensorData(std::shared_ptr<SensorData> pSensorData) {
  for (auto sensor = begin(*config_.sensors());
       sensor != end(*config_.sensors());
       ++sensor) {
    std::string sensorName = *sensor->sensorName();
    if (simulatedSensorRead_.find(sensorName) != simulatedSensorRead_.end()) {
      pSensorData->updateEntryFloat(
          sensorName, simulatedSensorRead_[sensorName], currentTimeStampSec_);
      facebook::fboss::FbossError(
          "Mokujin sensor read failure simulated : ", sensorName);
    } else {
      XLOG(ERR) << "Failed to read sensor : " << *sensor->sensorName();
    }
  }
  return;
}
int Mokujin::emergencyShutdown(bool /* enable */) {
  int rc = 0;
  setEmergencyState(true);
  oFs_ << std::to_string(currentTimeStampSec_) << "::"
       << "SYSTEM"
       << "::"
       << "Emergency Shutdown Sequence Executed" << std::endl;
  return rc;
}

float Mokujin::readSysfs(std::string path) const {
  float retVal = 0;

  auto it = simulatedSensorRead_.find(path);
  if (it != simulatedSensorRead_.end()) {
    auto rawVal = it->second;
    retVal = static_cast<float>(rawVal);
  } else {
    throw facebook::fboss::FbossError(
        "Mokujin :: sysfs entry not found : ", path);
  }
  return retVal;
}

bool Mokujin::writeSysfs(std::string path, int value) {
  oFs_ << std::to_string(currentTimeStampSec_) << "::" << path << "::"
       << "Set to " << std::to_string(value) << std::endl;
  updateSimulationData(path, static_cast<float>(value));
  return true;
}

int Mokujin::run(const std::string& cmd) {
  oFs_ << std::to_string(currentTimeStampSec_) << ":: util :: " << cmd
       << std::endl;
  return 0;
}

uint64_t Mokujin::getCurrentTime() const {
  return currentTimeStampSec_;
}

bool Mokujin::setFanShell(
    std::string command,
    std::string keySymbol,
    std::string fanName,
    int pwm) {
  std::string pwmStr = std::to_string(pwm);
  command = replaceAllString(command, "_NAME_", fanName);
  command = replaceAllString(command, keySymbol, pwmStr);
  oFs_ << std::to_string(currentTimeStampSec_) << "::" << fanName
       << "::" << command << std::endl;
  return true;
}

int Mokujin::getNextEventTime() {
  return nextEventTimeSec_;
}

bool Mokujin::hasAnyMoreEvent(uint64_t timeInSec) {
  if (nextEventTimeSec_ <= timeInSec)
    return true;
  return false;
}
bool Mokujin::getNextSensorEvent(
    uint64_t timeSec,
    std::string& key,
    float& value) {
  currentTimeStampSec_ = timeSec;
  if (isEof())
    return false;
  if (!hasAnyMoreEvent(timeSec))
    return false;
  if (nextEventDeadSensor_) {
    // If the event is to kill the sensor,
    // we return "" as sensor name, and remove the entry from hash table,
    // so that the sensor read would fail next time.
    if (simulatedSensorRead_.find(nextEventSensor_) !=
        simulatedSensorRead_.end()) {
      simulatedSensorRead_.erase(nextEventSensor_);
    }
    nextEventDeadSensor_ = false;
    key = "";
    value = 0.0;
  } else {
    // Normal sensor event
    key = nextEventSensor_;
    value = nextEventValue_;
    XLOG(INFO) << "Mokujin :: getNextSensorEvent [" << key << "," << value
               << "]";
  }
  // Clear the next event name, and fill up with the next event
  // if there are more events left in the input file
  nextEventSensor_ = "";
  if (!iFs_.eof() && key != "end") {
    std::string nextLine;
    getline(iFs_, nextLine);
    std::vector<std::string> token;
    for (auto i = strtok(&nextLine[0], "::"); i != NULL;
         i = strtok(NULL, "::")) {
      token.push_back(i);
    }
    nextEventTimeSec_ = (uint64_t)stoi(token[0]);
    nextEventSensor_ = token[1];
    std::string lowerStr = "";
    for (int i = 0; i < token[2].size(); i++) {
      lowerStr += tolower(token[2][i]);
    }
    if (lowerStr == "dead") {
      nextEventValue_ = 0.0;
      nextEventDeadSensor_ = true;
    } else {
      nextEventValue_ = stof(token[2]);
      nextEventDeadSensor_ = false;
    }
    XLOG(INFO) << "Mokujin :: getNextSensorEvent Upcoming Event : "
               << nextEventTimeSec_ << "," << nextEventSensor_ << ","
               << nextEventValue_ << ","
               << (nextEventDeadSensor_ ? "Dead" : "Alive") << std::endl;
  } else {
    XLOG(INFO)
        << "Mokujin :: getNextSensorEvent Upcoming Event : None, due to EOF";
  }
  XLOG(INFO) << "Mokujin :: Returning sensor event : " << key << "," << value;
  return true;
}

bool Mokujin::isEof() {
  return ((nextEventSensor_ == "") && (iFs_.eof()));
}

bool Mokujin::bothFileOpen() {
  return (iFOpen_ && oFOpen_);
}

bool Mokujin::initializeQsfpService() {
  // Do nothing in Mock. Just return success
  return true;
}

void Mokujin::getOpticsData(std::shared_ptr<SensorData> pSensorData) {
  // Basically, for each qsfpgroup if <name>Speed and <name>Temp both exists,
  // use this to fill up the table For each optics group, check if the key for
  // table type and table temperature
  for (auto opticGroup = config_.optics()->begin();
       opticGroup != config_.optics()->end();
       ++opticGroup) {
    std::string opticTypeKey = *opticGroup->opticName() + "TYPE";
    std::string opticTempKey = *opticGroup->opticName() + "TEMP";
    // If both key exists, then fetch the value, otherwise, just move on.
    if ((simulatedSensorRead_.find(opticTypeKey) !=
         simulatedSensorRead_.end()) &&
        (simulatedSensorRead_.find(opticTempKey) !=
         simulatedSensorRead_.end())) {
      int typeInt = static_cast<int>(simulatedSensorRead_[opticTypeKey]);
      float value = simulatedSensorRead_[opticTempKey];
      std::pair<fan_config_structs::OpticTableType, float> prepData = {
          static_cast<fan_config_structs::OpticTableType>(typeInt), value};
      // Add the data, and set the timestamp
      pSensorData->setLastQsfpSvcTime(getCurrentTime());
      OpticEntry* opticData =
          pSensorData->getOrCreateOpticEntry(*opticGroup->opticName());
      opticData->data.clear();
      opticData->data.push_back(prepData);
      opticData->lastOpticsUpdateTimeInSec = getCurrentTime();
      opticData->dataProcessTimeStamp = 0;
      opticData->calculatedPwm = 0;
    }
  }
  return;
}

} // namespace facebook::fboss::platform
