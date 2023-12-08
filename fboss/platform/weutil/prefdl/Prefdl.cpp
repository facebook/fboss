// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/prefdl/Prefdl.h"
#include <folly/Conv.h>
#include <fstream>
#include <iostream>

namespace facebook::fboss::platform {

PrefdlBase::PrefdlBase(const std::string& fileName) {
  std::ifstream iFile(fileName, std::ios::in | std::ios::binary);
  if (iFile) {
    strStream_ << iFile.rdbuf();
    iFile.close();
  } else {
    throw std::runtime_error("Cannot open file: " + fileName);
  }
}

void PrefdlBase::parseData() {
  fields_.clear();
  dict_.clear();

  strStream_.read(strBuf_.data(), 4);

  fields_.push_back("Prefdl Version");
  dict_["Prefdl Version"] = strBuf_;

  if (!strStream_) {
    throw std::runtime_error("Cannot read Prefdl Version: ");
  }

  preParse();
  while (parseTlvField() && !strStream_.eof()) {
  }
  checkCrc();
}

void PrefdlBase::printData() {
  if (fields_.empty()) {
    parseData();
  }
  for (auto key : fields_) {
    std::cout << key << ": " << dict_[key] << std::endl;
  }
}

std::string PrefdlBase::getField(const std::string& fieldName) {
  if (dict_.empty()) {
    parseData();
  }

  if (fieldName == "HwVer") {
    std::string hwRev = dict_["HwRev"];
    return hwRev.substr(0, hwRev.find('.'));
  }
  if (fieldName == "HwSubVer") {
    std::string hwRev = dict_["HwRev"];
    return hwRev.substr(hwRev.find('.') + 1);
  }

  if (dict_.find(fieldName) == dict_.end()) {
    return "";
  }

  return dict_[fieldName];
}

void PrefdlBase::preParse() {
  parseFixedField(0x0d);
  parseFixedField(0x0e);
  parseFixedField(0x0f);
}

bool PrefdlBase::parseTlvField() {
  std::string sId(2, '\0');
  std::string sLen(4, '\0');
  std::string value = "";
  int len = 0;
  strStream_.read(&sId[0], 2);
  strStream_.read(&sLen[0], 4);

  len = std::stoi(sLen, nullptr, 16);

  if (len > 0) {
    value.resize(len);
    strStream_.read(&value[0], len);
  }

  strBuf_ += folly::to<std::string>(sId, sLen, value);

  int id = std::stoi(sId, nullptr, 16);

  if (id == 0) {
    return false;
  }

  auto search = table_.find(id);
  if (search != table_.end()) {
    if (search->second.first == "MAC") {
      value = parseMacField(value);
    }

    fields_.push_back(table_[id].first);
    dict_[table_[id].first] = value;
  }

  return true;
}

void PrefdlBase::checkCrc() {
  // ToDo
}

void PrefdlBase::parseFixedField(int code) {
  std::string value(table_[code].second, ' ');

  strStream_.read(&value[0], table_[code].second);

  strBuf_ += value;

  dict_[table_[code].first] = value;
  fields_.push_back(table_[code].first);
}

std::string PrefdlBase::parseMacField(const std::string& mac) {
  return mac.substr(0, 2) + ":" + mac.substr(2, 2) + ":" + mac.substr(4, 2) +
      ":" + mac.substr(6, 2) + ":" + mac.substr(8, 2) + ":" + mac.substr(10, 2);
}

} // namespace facebook::fboss::platform
