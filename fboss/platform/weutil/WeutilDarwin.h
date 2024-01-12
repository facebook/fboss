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
  void printInfoJson() override;

 private:
  void genSpiPrefdlFile();
  std::unique_ptr<PrefdlBase> eepromParser_;
};

} // namespace facebook::fboss::platform
