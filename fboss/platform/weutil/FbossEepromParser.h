// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "fboss/platform/weutil/FbossEepromInterface.h"

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
  std::unique_ptr<FbossEepromInterface> parseEepromBlobTLV(
      int eepromVer,
      const unsigned char* buffer,
      const int readCount);

  std::string parseLeUint(int len, unsigned char* ptr);
  std::string parseBeUint(int len, unsigned char* ptr);
  std::string parseLeHex(int len, unsigned char* ptr);
  std::string parseBeHex(int len, unsigned char* ptr);
  std::string parseString(int len, unsigned char* ptr);
  std::string parseMac(int len, unsigned char* ptr);
  std::string parseDate(int len, unsigned char* ptr);
  uint16_t calculateCrc16(const uint8_t* buffer, size_t len);

  std::string eepromPath_;
  uint16_t offset_;
};

} // namespace facebook::fboss::platform
