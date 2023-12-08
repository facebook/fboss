// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <gflags/gflags.h>
#include "fboss/platform/weutil/FbossEepromParser.h"
#include "fboss/platform/weutil/WeutilInterface.h"

namespace facebook::fboss::platform {

struct WeutilInfo {
  std::string eepromPath;
  int offset;
  std::vector<std::string> modules;
};

class WeutilImpl : public WeutilInterface {
  using EepromEntry = std::vector<std::pair<std::string, std::string>>;

 private:
  FbossEepromParser eepromParser;
  WeutilInfo info_;

 public:
  explicit WeutilImpl(const WeutilInfo eeprom);
  // This method will translate EEPROM blob to human readable format by
  // firstly translate it into <field_id, value> pair, then to
  // <key, value> pair
  std::vector<std::pair<std::string, std::string>> getInfo() override;
  void printInfo() override;
  void printInfoJson() override;
  std::vector<std::string> getEepromNames() const override;
};

} // namespace facebook::fboss::platform
