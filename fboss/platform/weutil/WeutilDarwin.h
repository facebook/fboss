// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <memory>

#include <gflags/gflags.h>

#include "fboss/platform/weutil/WeutilInterface.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

namespace facebook::fboss::platform {
class WeutilDarwin : public WeutilInterface {
 public:
  explicit WeutilDarwin(const std::string& eepromPath);

  std::vector<std::pair<std::string, std::string>> getContents() override;
  void printInfo() override;
  folly::dynamic getInfoJson() override;

 private:
  // Runs flashrom/dd to dump the SPI prefdl into the given per-run temp
  // directory and returns the path to the generated prefdl file.
  std::string genSpiPrefdlFile(const std::string& runDir);
  std::unique_ptr<PrefdlBase> eepromParser_;
};

} // namespace facebook::fboss::platform
