// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>
#include <vector>

namespace facebook::fboss::platform {
class WeutilInterface {
 public:
  WeutilInterface() {}
  virtual void printInfo() = 0;
  virtual void printInfoJson() = 0;
  // get weutil info in a vector of pairs, e.g. <"Version", "x"> , etc
  virtual std::vector<std::pair<std::string, std::string>> getInfo() = 0;
  virtual ~WeutilInterface() = default;

 protected:
  // wetuil output fields and default value for all FBOSS switches w/wo OpenBMC
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
