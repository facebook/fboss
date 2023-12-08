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
  void printUsage(void) override;

 private:
  void genSpiPrefdlFile(void);
  std::string getEepromPathFromName(const std::string& name);

  std::unique_ptr<PrefdlBase> eepromParser_;
};

} // namespace facebook::fboss::platform
