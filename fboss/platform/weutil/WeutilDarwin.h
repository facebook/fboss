// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once
#include <gflags/gflags.h>
#include "fboss/platform/weutil/WeutilInterface.h"

namespace facebook::fboss::platform {
class WeutilDarwin : public WeutilInterface {
 public:
  WeutilDarwin(std::string eeprom = "");
  std::vector<std::pair<std::string, std::string>> getInfo(
      std::string eeprom = "") override;
  void printInfo() override;
  void printInfoJson() override;
  bool verifyOptions(void) override;

 private:
  void genSpiPrefdlFile(void);
  void printUsage(void);
  std::string eeprom_;
};

} // namespace facebook::fboss::platform
