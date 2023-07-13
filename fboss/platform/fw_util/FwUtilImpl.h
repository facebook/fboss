/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/Subprocess.h>
#include <folly/system/Shell.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include "fboss/platform/fw_util/if/gen-cpp2/fw_util_config_types.h"

namespace facebook::fboss::platform::fw_util {

using namespace facebook::fboss::platform::fw_util_config;

class FwUtilImpl {
 public:
  explicit FwUtilImpl(const std::string& confFileName)
      : confFileName_{confFileName} {
    init();
  }
  void printVersion(const std::string&);
  bool isFilePresent(const std::string&);
  void doFirmwareAction(const std::string&, const std::string&);
  std::string toLower(std::string);
  std::string runVersionCmd(const std::string&);
  void checkCmdStatus(const std::string&, int);
  int runCmd(const std::string&);
  void storeFilePath(const std::string&, const std::string&);
  void doPreUpgrade(const std::string&, const std::string&);
  std::string printFpdList();

 private:
  // Firmware config file full path
  std::string confFileName_{};

  FwUtilConfig fwUtilConfig_{};

  void init();

  // use to map firmware device with priority for cases where
  // we have to upgrade all the firmware device at once and we
  // have to take the priority into consideration

  std::vector<std::pair<std::string, int>> fwDeviceNamesByPrio_;
};

} // namespace facebook::fboss::platform::fw_util
