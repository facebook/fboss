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
#include <folly/logging/xlog.h>
#include <folly/system/Shell.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include "fboss/platform/fw_util/Flags.h"
#include "fboss/platform/fw_util/if/gen-cpp2/fw_util_config_types.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::fw_util {

using namespace facebook::fboss::platform::fw_util_config;

class FwUtilImpl {
 public:
  explicit FwUtilImpl() {
    init();
  }
  void doVersionAudit();
  std::string printFpdList();
  void doFirmwareAction(const std::string&, const std::string&);
  void printVersion(const std::string&);

 private:
  std::string runVersionCmd(const std::string&);
  void doPreUpgrade(const std::string&);

  void printAllVersions();
  std::string getSingleVersion(const std::string&);
  void doPreUpgradeOperation(
      const PreFirmwareOperationConfig&,
      const std::string&);

  void storeFlashromConfig(const FlashromConfig&, const std::string&);
  void doJtagOperation(const JtagConfig&, const std::string&);
  void doGpiosetOperation(const GpiosetConfig&, const std::string&);
  void doUpgrade(const std::string&);
  void performFlashromUpgrade(const FlashromConfig&, const std::string&);
  void addCommonFlashromArgs(
      const FlashromConfig&,
      const std::string&,
      const std::string&,
      std::vector<std::string>&);
  void setProgrammerAndChip(
      const FlashromConfig&,
      const std::string&,
      std::vector<std::string>&);
  void addLayoutFile(
      const FlashromConfig&,
      std::vector<std::string>&,
      const std::string&);
  bool createCustomContentFile(const std::string&, const int &, const std::string&);
  std::string detectFlashromChip(const FlashromConfig&, const std::string&);
  void performJamUpgrade(const JamConfig&, const std::string&);
  void performXappUpgrade(const XappConfig&, const std::string&);
  void doPostUpgrade(const std::string&);
  void doPostUpgradeOperation(
      const PostFirmwareOperationConfig&,
      const std::string&);
  void doGpiogetOperation(const GpiogetConfig&, const std::string&);
  void performRead(const ReadFirmwareOperationConfig&, const std::string& fpd);
  void performReadOperation(
      const ReadFirmwareOperationConfig&,
      const std::string&);
  void performFlashromRead(const FlashromConfig&, const std::string&);
  void addFileOption(
      const std::string&,
      std::vector<std::string>&,
      std::optional<std::string>&);
  void performFlashromVerify(const FlashromConfig&, const std::string&);
  void performVerify(const VerifyFirmwareOperationConfig&, const std::string&);
  void doWriteToPortOperation(const WriteToPortConfig&, const std::string&);
  // TODO: Remove those prototypes once we move darwin to PM and
  //  have the latest drivers running
  std::string runVersionCmdDarwin(const std::string&);
  void printDarwinVersion(const std::string&);
  void performUpgradeOperation(const UpgradeConfig&, const std::string&);
  void doUpgradeOperation(const UpgradeConfig&, const std::string&);

  NewFwUtilConfig fwUtilConfig_{};
  std::map<std::string, std::vector<std::string>> spiChip_;
  std::string platformName_;

  void init();

  // use to map firmware device with priority for cases where
  // we have to upgrade all the firmware device at once and we
  // have to take the priority into consideration

  std::vector<std::pair<std::string, int>> fwDeviceNamesByPrio_;
};

} // namespace facebook::fboss::platform::fw_util
