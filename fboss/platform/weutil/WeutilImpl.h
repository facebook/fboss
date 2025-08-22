// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gflags/gflags.h>

#include "fboss/platform/weutil/WeutilInterface.h"

namespace facebook::fboss::platform {

class WeutilImpl : public WeutilInterface {
 public:
  explicit WeutilImpl(const std::string& eepromPath, const uint16_t offset);
  std::vector<std::pair<std::string, std::string>> getContents() override;
  void printInfo() override;
  void printInfoJson() override;

 private:
  const std::string eepromPath_;
  const uint16_t offset_;
};

} // namespace facebook::fboss::platform
