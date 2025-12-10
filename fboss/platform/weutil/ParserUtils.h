// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary

#pragma once

#include <string>
#include <vector>

namespace facebook::fboss::platform {

class ParserUtils {
 public:
  static std::vector<uint8_t> loadEeprom(const std::string& eeprom, int offset);
  static std::string parseBeUint(int len, unsigned char* ptr);
  static std::string parseBeHex(int len, unsigned char* ptr);
  static std::string parseString(int len, unsigned char* ptr);
  static std::string parseMac(int len, unsigned char* ptr);
  static uint16_t calculateCrc16(const uint8_t* buffer, size_t len);
};
} // namespace facebook::fboss::platform
