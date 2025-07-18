/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <filesystem>
#include <iostream>

#include <fmt/core.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/fw_util/fw_util_helpers.h"
#include "fboss/platform/helpers/PlatformNameLib.h"

using namespace folly::literals::shell_literals;

namespace facebook::fboss::platform::fw_util {
void FwUtilImpl::init() {
  platformName_ = helpers::PlatformNameLib().getPlatformName().value();
  ConfigLib configLib(configFilePath_);
  std::string fwUtilConfJson = configLib.getFwUtilConfig();
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<NewFwUtilConfig>(
        fwUtilConfJson, fwUtilConfig_);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Error deserializing NewFwUtilConfig: " << e.what();
  }
  for (const auto& [fpd, fwConfigs] : *fwUtilConfig_.newFwConfigs()) {
    fwDeviceNamesByPrio_.emplace_back(fpd, *fwConfigs.priority());
  }
  // Sort firmware devices by priority
  std::sort(
      fwDeviceNamesByPrio_.begin(),
      fwDeviceNamesByPrio_.end(),
      [](const auto& rhsFwDevice, const auto& lhsFwDevice) {
        return rhsFwDevice.second < lhsFwDevice.second;
      });

  fwUtilVersionHandler_ = std::make_unique<FwUtilVersionHandler>(
      fwDeviceNamesByPrio_, fwUtilConfig_);
}

std::string FwUtilImpl::printFpdList() {
  // get list of firmware devide name
  std::string fpdList;
  for (const auto& fpd : fwDeviceNamesByPrio_) {
    fpdList += fpd.first + " ";
  }

  return fpdList;
}

void FwUtilImpl::doFirmwareAction(
    const std::string& fpd,
    const std::string& action) {
  XLOG(INFO, " Analyzing the given FW action for execution");

  auto iter = fwUtilConfig_.newFwConfigs()->find(fpd);
  if (iter == fwUtilConfig_.newFwConfigs()->end()) {
    XLOG(INFO)
        << fpd
        << " is not part of the firmware target_name list Please run ./fw-util --helpon=Flags for the right usage";
    exit(1);
  }
  auto fwConfig = iter->second;

  if (action == "program") {
    // Fw_util is build as part of ramdisk once every 24 hours
    // assuming no test failure. if we force sha1sum check in the fw_util,
    // we will end up blocking provisioning until a new ramdisk is built
    // by their conveyor which can take more than 24 hours if there are test
    // failures. Hence, we are adding a flag to make sha1sum check optional.
    // We will enforce sha1sum check in the run_script of the FIS packages.

    if (verifySha1sum_) {
      if (fwConfig.sha1sum().has_value()) {
        verifySha1sum(fpd, *fwConfig.sha1sum(), fwBinaryFile_);
      } else {
        XLOG(WARN) << "sha1sum is not set in the " << platformName_
                   << " config file. Skipping the action.";
        return;
      }
    }

    if (fwConfig.preUpgrade().has_value()) {
      doPreUpgrade(fpd);
    }

    if (fwConfig.upgrade().has_value()) {
      // Perform the upgrade operation
      doUpgrade(fpd);
    } else {
      XLOG(ERR) << "upgrade entry is not set in the json";
    }

    // do post upgrade operation
    if (fwConfig.postUpgrade().has_value()) {
      doPostUpgrade(fpd);
    }
  } else if (action == "read" && fwConfig.read().has_value()) {
    // do pre upgrade operation
    if (fwConfig.preUpgrade().has_value()) {
      doPreUpgrade(fpd);
    }

    performRead(fwConfig.read().value(), fpd);

    // do post upgrade operation
    if (fwConfig.postUpgrade().has_value()) {
      doPostUpgrade(fpd);
    }
  } else if (action == "verify" && fwConfig.verify().has_value()) {
    // do pre upgrade operation
    if (fwConfig.preUpgrade().has_value()) {
      doPreUpgrade(fpd);
    }

    performVerify(fwConfig.verify().value(), fpd);

    // do post upgrade operation
    if (fwConfig.postUpgrade().has_value()) {
      doPostUpgrade(fpd);
    }
  } else {
    XLOG(INFO) << "Invalid action: " << action
               << ". Please run ./fw-util --help Flags for the right usage";
    exit(1);
  }
}

void FwUtilImpl::printVersion(const std::string& fpd) {
  // TODO: Remove this check once we have moved all Darwin systems to the latest
  // BSP which provide a single sysfs endpoint for each firmware version
  auto lowerCasePlatformName = toLower(platformName_);

  if (lowerCasePlatformName == "darwin") {
    fwUtilVersionHandler_->printDarwinVersion(fpd);
  } else {
    if (fpd == "all") {
      fwUtilVersionHandler_->printAllVersions();
    } else {
      std::string version = fwUtilVersionHandler_->getSingleVersion(fpd);
      std::cout << fmt::format("{} : {}", fpd, version) << std::endl;
    }
  }
}

void FwUtilImpl::doVersionAudit() {
  bool mismatch = false;
  for (const auto& [fpdName, fwConfig] : *fwUtilConfig_.newFwConfigs()) {
    std::string desiredVersion = *fwConfig.desiredVersion();
    if (desiredVersion.empty()) {
      XLOGF(
          INFO,
          "{} does not have a desired version specified in the config.",
          fpdName);
      continue;
    }
    std::string actualVersion;
    try {
      actualVersion = fwUtilVersionHandler_->getSingleVersion(fpdName);
    } catch (const std::exception& e) {
      XLOG(ERR) << "Failed to get version for " << fpdName << ": " << e.what();
      continue;
    }

    if (actualVersion != desiredVersion) {
      XLOGF(
          INFO,
          "{} is at version {} which does not match config's desired version {}.",
          fpdName,
          actualVersion,
          desiredVersion);
      mismatch = true;
    }
  }
  if (mismatch) {
    XLOG(INFO, "Firmware version mismatch found.");
    exit(1);
  } else {
    XLOG(INFO, "All firmware versions match the config.");
  }
}
} // namespace facebook::fboss::platform::fw_util
