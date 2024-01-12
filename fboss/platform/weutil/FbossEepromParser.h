// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::fboss::platform {

class FbossEepromParser {
 public:
  explicit FbossEepromParser(
      const std::string& eepromPath,
      const uint16_t offset)
      : eepromPath_(eepromPath), offset_(offset) {}

  std::vector<std::pair<std::string, std::string>> getContents();

 private:
  int loadEeprom(
      const std::string& eeprom,
      unsigned char* output,
      int offset,
      int max);
  std::unordered_map<int, std::string> parseEepromBlobLinear(
      const unsigned char* buffer);
  std::unordered_map<int, std::string> parseEepromBlobTLV(
      int eepromVer,
      const unsigned char* buffer,
      const int readCount);

  // This method is a helper function to translate <field_id, value> pair
  // into <field_name, field_value> pair, so that the user of this
  // methon can parse the data into human readable screen output or
  // JSON
  std::vector<std::pair<std::string, std::string>> prepareEepromFieldMap(
      std::unordered_map<int, std::string> parsedValue,
      int eepromVer);
  std::string parseLeUint(int len, unsigned char* ptr);
  std::string parseBeUint(int len, unsigned char* ptr);
  std::string parseLeHex(int len, unsigned char* ptr);
  std::string parseBeHex(int len, unsigned char* ptr);
  std::string parseString(int len, unsigned char* ptr);
  std::string parseV4Mac(int len, unsigned char* ptr);
  std::string parseV5Mac(int len, unsigned char* ptr);
  std::string parseLegacyMac(int len, unsigned char* ptr);
  std::string parseDate(int len, unsigned char* ptr);

  std::string eepromPath_;
  uint16_t offset_;
};

} // namespace facebook::fboss::platform
