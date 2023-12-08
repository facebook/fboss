// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <memory>

#include <gflags/gflags.h>

#include "fboss/platform/weutil/WeutilInterface.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

namespace facebook::fboss::platform {
class WeutilDarwin : public WeutilInterface {
 public:
  WeutilDarwin(const std::string& eeprom = "");
  std::vector<std::pair<std::string, std::string>> getInfo() override;
  void printInfo() override;
  void printInfoJson() override;
  std::vector<std::string> getEepromNames() const override;

 private:
  void genSpiPrefdlFile();
  std::string getEepromPathFromName(const std::string& name);

  std::unique_ptr<PrefdlBase> eepromParser_;
};

} // namespace facebook::fboss::platform
