// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
namespace facebook::fboss::platform {

// For parsing system metadata (known as prefdl) for selected Darwin switches
// The prefdl is a block of ASCII text, decoded into a set of key:value pairs
class PrefdlBase {
 public:
  explicit PrefdlBase(const std::string& fileName);

  void printData();
  std::string getField(const std::string&);

  // ToDo: add method to get individual field
 private:
  void parseData();
  void preParse();
  bool parseTlvField();
  void parseFixedField(int code);
  std::string parseMacField(const std::string& mac);
  void checkCrc();

  std::stringstream strStream_;
  std::string strBuf_{"0000"};
  std::vector<std::string> fields_;
  std::unordered_map<std::string, std::string> dict_;

  // Map to parse prefdl fields:  {ID: [description, length]}. If length is -1,
  // then the length will come from prefdl
  std::unordered_map<int, std::pair<std::string, int>> table_{
      {0x00, std::make_pair("END", 0)},
      {0x01, std::make_pair("Deviation", -1)},
      {0x02, std::make_pair("MfgTime", -1)},
      {0x03, std::make_pair("SKU", -1)},
      {0x04, std::make_pair("ASY", -1)},
      {0x05, std::make_pair("MAC", -1)},
      {0x0a, std::make_pair("HwApi", -1)},
      {0x0b, std::make_pair("HwRev", -1)},
      {0x0c, std::make_pair("SID", -1)},
      {0x0d, std::make_pair("PCA", 12)},
      {0x0e, std::make_pair("SerialNumber", 11)},
      {0x0f, std::make_pair("KVN", 3)},
      {0x17, std::make_pair("MfgTime2", -1)},
  };
};
} // namespace facebook::fboss::platform
