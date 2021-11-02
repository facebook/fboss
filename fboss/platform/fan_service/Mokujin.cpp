// Copyright 2021- Facebook. All rights reserved.

// Implementation of Mokujin class. Refer to .h file
// for functional description.
#include "Mokujin.h"

Mokujin::Mokujin() {
  iFOpen_ = false;
  oFOpen_ = false;
  nextEventDeadSensor_ = false;
  currentTimeStampSec_ = 0;
  return;
}
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
void Mokujin::getSensorData(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    std::shared_ptr<SensorData> pSensorData) {
  for (auto sensor = begin(pServiceConfig->sensors);
       sensor != end(pServiceConfig->sensors);
       ++sensor) {
    std::string sensorName = sensor->sensorName;
    if (simulatedSensorRead_.find(sensorName) != simulatedSensorRead_.end()) {
      pSensorData->updateEntryFloat(
          sensorName, simulatedSensorRead_[sensorName], currentTimeStampSec_);
      facebook::fboss::FbossError(
          "Mokujin sensor read failure simulated : ", sensorName);
    } else {
      XLOG(ERR) << "Failed to read sensor : " << sensor->sensorName;
    }
  }
  return;
}
int Mokujin::emergencyShutdown(
    std::shared_ptr<ServiceConfig> pServiceConfig,
    bool enable) {
  int rc = 0;
  emergencyShutdownState = true;
  oFs_ << std::to_string(currentTimeStampSec_) << "::"
       << "SYSTEM"
       << "::"
       << "Emergency Shutdown Sequence Executed";
  return rc;
}

bool Mokujin::setFanPwmSysfs(std::string path, int pwm) {
  oFs_ << std::to_string(currentTimeStampSec_) << "::" << path << "::"
       << "Set to " << std::to_string(pwm);
  return true;
}

uint64_t Mokujin::getCurrentTime() {
  return currentTimeStampSec_;
}

bool Mokujin::setFanPwmShell(
    std::string command,
    std::string fanName,
    int pwm) {
  std::string pwmStr = std::to_string(pwm);
  command = replaceAllString(command, "_NAME_", fanName);
  command = replaceAllString(command, "_PWM_", pwmStr);
  oFs_ << std::to_string(currentTimeStampSec_) << "::" << fanName
       << "::" << command;
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
