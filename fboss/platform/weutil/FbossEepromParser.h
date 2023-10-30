// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::fboss::platform {

typedef std::vector<std::pair<std::string, std::string>> parsedEepromEntry;

class FbossEepromParser {
 private:
  int loadEeprom(
      const std::string& eeprom,
      unsigned char* output,
      int offset,
      int max);
  std::unordered_map<int, std::string> parseEepromBlobV3(
      const unsigned char* buffer);
  std::unordered_map<int, std::string> parseEepromBlobV4(
      const unsigned char* buffer,
      const int readCount);
  parsedEepromEntry getAllInfo(const std::string& eeprom, const int offset);
  // This method is a helper function to translate <field_id, value> pair
  // into <field_name, field_value> pair, so that the user of this
  // methon can parse the data into human readable screen output or
  // JSON
  std::vector<std::pair<std::string, std::string>> prepareEepromFieldMap(
      std::unordered_map<int, std::string> parsedValue,
      int eepromVer);
  std::string parseUint(int len, unsigned char* ptr);
  std::string parseHex(int len, unsigned char* ptr);
  std::string parseString(int len, unsigned char* ptr);
  std::string parseMac(int len, unsigned char* ptr);
  std::string parseLegacyMac(int len, unsigned char* ptr);
  std::string parseDate(int len, unsigned char* ptr);

 public:
  parsedEepromEntry getEeprom(const std::string& eepromPath, const int offset);
};

} // namespace facebook::fboss::platform
