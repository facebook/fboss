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
  std::vector<std::string> getModuleNames();

 public:
  explicit WeutilImpl(const WeutilInfo eeprom);
  // This method will translate EEPROM blob to human readable format by
  // firstly translate it into <field_id, value> pair, then to
  // <key, value> pair
  std::vector<std::pair<std::string, std::string>> getInfo(
      const std::string& eeprom = "") override;
  void printInfo() override;
  void printInfoJson() override;
  bool getEepromPath(void) override;
  void printUsage(void) override;
};

} // namespace facebook::fboss::platform
