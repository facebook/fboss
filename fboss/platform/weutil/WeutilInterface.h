// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::fboss::platform {

// WeutilImpl will be used as is in BMC codebase too, where we do not have
// thrift and folly support. Therefore we define the following structure for
// WeutilImpl. In fbcode, we will still use thrift and folly to parse the config
// but then store the data in this "plain" struct that does not require thrift.
typedef struct {
  std::string chassisEeprom;
  // Mapping fru name (string) to the tuple of <Path, Offset, Version>
  std::unordered_map<std::string, std::tuple<std::string, int, int>>
      configEntry;
} PlainWeutilConfig;

class WeutilInterface {
 public:
  WeutilInterface() {}
  virtual void printInfo() = 0;
  virtual void printInfoJson() = 0;
  virtual bool getEepromPath(void) = 0;
  // get weutil info in a vector of pairs, e.g. <"Version", "x"> , etc
  virtual std::vector<std::pair<std::string, std::string>> getInfo(
      const std::string& eeprom = "") = 0;
  virtual ~WeutilInterface() = default;
  virtual void printUsage() = 0;

 protected:
  // weutil output fields and default value for all FBOSS switches w/wo OpenBMC
  const std::vector<std::pair<std::string, std::string>> weFields_{
      {"Wedge EEPROM", "CHASSIS"},
      {"Version", "0"},
      {"Product Name", ""},
      {"Product Part Number", ""},
      {"System Assembly Part Number", ""},
      {"Facebook PCBA Part Number", ""},
      {"Facebook PCB Part Number", ""},
      {"ODM PCBA Part Number", ""},
      {"ODM PCBA Serial Number", ""},
      {"Product Production State", "0"},
      {"Product Version", ""},
      {"Product Sub-Version", ""},
      {"Product Serial Number", ""},
      {"Product Asset Tag", ""},
      {"System Manufacturer", ""},
      {"System Manufacturing Date", ""},
      {"PCB Manufacturer", ""},
      {"Assembled At", ""},
      {"Local MAC", ""},
      {"Extended MAC Base", ""},
      {"Extended MAC Address Size", ""},
      {"Location on Fabric", ""},
      {"CRC8", "0x0"},
  };
};

} // namespace facebook::fboss::platform
