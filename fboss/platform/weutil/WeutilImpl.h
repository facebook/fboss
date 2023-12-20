// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <gflags/gflags.h>
#include "fboss/platform/weutil/FbossEepromParser.h"
#include "fboss/platform/weutil/WeutilInterface.h"

namespace facebook::fboss::platform {

class WeutilImpl : public WeutilInterface {
  using EepromEntry = std::vector<std::pair<std::string, std::string>>;

 private:
  FbossEepromParser eepromParser;
  std::string eepromPath_;
  int offset_;

 public:
  explicit WeutilImpl(const std::string& eepromPath, const int offset);
  // This method will translate EEPROM blob to human readable format by
  // firstly translate it into <field_id, value> pair, then to
  // <key, value> pair
  std::vector<std::pair<std::string, std::string>> getInfo() override;
  void printInfo() override;
  void printInfoJson() override;
};

} // namespace facebook::fboss::platform
