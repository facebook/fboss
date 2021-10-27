// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include "fboss/platform/weutil/WeutilInterface.h"
#include "fboss/platform/weutil/prefdl/prefdl.h"

namespace facebook::fboss::platform {
class WeutilDarwin : public WeutilInterface {
 public:
  WeutilDarwin();
  virtual void printInfo() override;
  virtual ~WeutilDarwin() override = default;

 protected:
  const std::string kPathPrefix_ = "/tmp/WeutilDarwin/";
  const std::string kPredfl_ = kPathPrefix_ + "system-prefdl-bin";
  const std::string kCreateLayout_ =
      "echo \"00001000:0001efff prefdl\" > " + kPathPrefix_ + "layout";
  const std::string kFlashRom_ = "flashrom -p internal -l " + kPathPrefix_ +
      "layout -i prefdl -r " + kPathPrefix_ + "/bios > /dev/null 2>&1";
  const std::string kddComands_ = "dd if=" + kPathPrefix_ +
      "/bios of=" + kPredfl_ + " bs=1 skip=8192 count=61440 > /dev/null 2>&1";

  // Map weutil fields to prefld fields
  const std::unordered_map<std::string, std::string> kMapping_{

      {"Product Name", "SID"},
      {"Product Part Number", "SKU"},
      {"System Assembly Part Number", "ASY"},
      {"ODM PCBA Part Number", "PCA"},
      {"Product Version", "HwVer"},
      {"Product Sub-Version", "HwSubVer"},
      {"System Manufacturing Date", "MfgTime2"},
      {"Local MAC", "MAC"},
      {"Product Serial Number", "SerialNumber"},
  };
};

} // namespace facebook::fboss::platform
