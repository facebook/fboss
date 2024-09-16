/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <sys/wait.h>
#include <filesystem>
#include <iostream>

#include <fmt/core.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fw_util/Flags.h"
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/helpers/PlatformUtils.h"

using namespace folly::literals::shell_literals;
namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::init() {
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  std::string fwUtilConfJson = ConfigLib().getFwUtilConfig(platformName);
  apache::thrift::SimpleJSONSerializer::deserialize<FwUtilConfig>(
      fwUtilConfJson, fwUtilConfig_);

  for (const auto& fpd : *fwUtilConfig_.fwConfigs()) {
    fwDeviceNamesByPrio_.emplace_back(fpd.first, *fpd.second.priority());
  }

  // sort fwDevice based on priority where the fpd
  // with the lowest priority means it should be
  // programmed first for instances where we run
  // the fw_util once to program all the fpd

  std::sort(
      fwDeviceNamesByPrio_.begin(),
      fwDeviceNamesByPrio_.end(),
      [](const auto& rhsFwDevice, const auto& lhsFwDevice) {
        return rhsFwDevice.second < lhsFwDevice.second;
      });
}

std::string FwUtilImpl::printFpdList() {
  // get list of firmware devide name
  std::string fpdList;

  for (const auto& fpd : fwDeviceNamesByPrio_) {
    fpdList += fpd.first + " ";
  }

  return fpdList;
}

std::string FwUtilImpl::runVersionCmd(const std::string& cmd) {
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error("Run " + cmd + " failed!");
  }
  return standardOut;
}

void FwUtilImpl::printVersion(const std::string& fpd) {
  if (fpd == "all") {
    for (const auto& orderedfpd : fwDeviceNamesByPrio_) {
      auto iter = fwUtilConfig_.fwConfigs()->find(orderedfpd.first);
      std::string version = runVersionCmd(*iter->second.getVersionCmd());
      std::cout << orderedfpd.first << " : " << version;
    }

  } else {
    auto iter = fwUtilConfig_.fwConfigs()->find(fpd);
    if (iter != fwUtilConfig_.fwConfigs()->end()) {
      std::string version = runVersionCmd(*iter->second.getVersionCmd());
      std::cout << fpd << " : " << version;
    } else {
      XLOG(INFO)
          << fpd
          << " is not part of the firmware target_name list Please use ./fw-util --helpon=Flags for the right usage";
    }
  }
}

bool FwUtilImpl::isFilePresent(const std::string& file) {
  return std::filesystem::exists(file);
}

std::string FwUtilImpl::toLower(std::string str) {
  std::string lowerCaseStr = str;
  folly::toLowerAscii(lowerCaseStr);
  return lowerCaseStr;
}

void FwUtilImpl::checkCmdStatus(const std::string& cmd, int exitStatus) {
  if (exitStatus < 0) {
    throw std::runtime_error(
        "Running command " + cmd + " failed with errno " +
        folly::errnoStr(errno));
  } else if (!WIFEXITED(exitStatus)) {
    throw std::runtime_error(
        "Running Command " + cmd + " exited abnormally " +
        std::to_string(WEXITSTATUS(exitStatus)));
  }
}

int FwUtilImpl::runCmd(const std::string& cmd) {
  auto shellCmd = "/bin/sh -c {}"_shellify(cmd);
  auto subProc = folly::Subprocess(shellCmd);
  auto ret = subProc.wait().exitStatus();
  return ret;
}

/*
 * For several fpd, pre-upgrade command must
 * be run before both reading a verify so that
 * we can get some details set up. This is quite
 * common in cases where we have to use flashrom.
 * Therefore we are extractin this in its own function
 * to prevent duplicate codes
 *
 */
void FwUtilImpl::doPreUpgrade(
    const std::string& fpd,
    const std::string& preUpgradeCmd) {
  int exitStatus = 0;
  if (!preUpgradeCmd.empty()) {
    XLOG(INFO) << "running preUpgradeCmd";
    exitStatus = runCmd(preUpgradeCmd);
    /*
     * For some platform, we have to determine the type of the chip
     * using the command flashrom -p internal which will always return
     * a non zero value which is expected. Therefore, checking the command
     * status for preUpgrade purpose (to determine the chip) will fail for bios
     * for some platform as expected. so we will skip it here.
     * */
    if (fpd != "bios") {
      checkCmdStatus(preUpgradeCmd, exitStatus);
    }
  }
}

void FwUtilImpl::verifySha1sum(
    const std::string& fpd,
    const std::string& configSha1sum) {
  const std::string cmd = folly::to<std::string>(
      "sha1sum ", FLAGS_fw_binary_file, " | cut -d ' ' -f 1");

  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error("Run" + cmd + " failed!");
  }
  standardOut.erase(
      std::remove(standardOut.begin(), standardOut.end(), '\n'),
      standardOut.end());
  if (configSha1sum != standardOut) {
    throw std::runtime_error(fmt::format(
        "{} config file sha1sum {} is different from current binary sha1sum of {}",
        fpd,
        configSha1sum,
        standardOut));
  }
}

void FwUtilImpl::doVersionAudit() {
  bool mismatch = false;
  for (const auto& [fpdName, fwConfig] : *fwUtilConfig_.fwConfigs()) {
    std::string desiredVersion = *fwConfig.desiredVersion();
    if (desiredVersion.empty()) {
      XLOGF(
          INFO,
          "{} does not have a desired version specified in the config.",
          fpdName);
      continue;
    }
    folly::StringPiece actualVersion =
        folly::trimWhitespace(runVersionCmd(*fwConfig.getVersionCmd()));
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

void FwUtilImpl::doFirmwareAction(
    const std::string& fpd,
    const std::string& action) {
  int exitStatus = 0;
  auto iter = fwUtilConfig_.fwConfigs()->find(fpd);
  if (iter == fwUtilConfig_.fwConfigs()->end()) {
    XLOG(INFO)
        << fpd
        << " is not part of the firmware target_name list Please run ./fw-util --helpon=Flags for the right usage";
    exit(1);
  }
  auto fwConfig = iter->second;

  const std::string upgradeCmd = *fwConfig.upgradeCmd();
  const std::string readFwCmd = *fwConfig.readFwCmd();
  const std::string verifyFwCmd = *fwConfig.verifyFwCmd();

  if (action == "program" && !upgradeCmd.empty()) {
    // Fw_util is build as part of ramdisk once every 24 hours
    // assuming no test failure. if we force sha1sum check in the fw_util,
    // we will end up blocking provisioning until a new ramdisk is built
    // by their conveyor which can take more than 24 hours if there are test
    // failures. Hence, we are adding a flag to make sha1sum check optional. We
    // will enforce sha1sum check in the run_script of the FIS packages.

    if (FLAGS_verify_sha1sum && !fwConfig.sha1sum()->empty()) {
      verifySha1sum(fpd, *fwConfig.sha1sum());
    }
    if (fwConfig.preUpgradeCmd().has_value()) {
      doPreUpgrade(fpd, *fwConfig.preUpgradeCmd());
    }
    XLOG(INFO) << "running upgradeCmd";
    exitStatus = runCmd(upgradeCmd);
    checkCmdStatus(upgradeCmd, exitStatus);
    if (fwConfig.postUpgradeCmd().has_value()) {
      XLOG(INFO) << "running postUpgradeCmd";
      const std::string postUpgradeCmd = *fwConfig.postUpgradeCmd();
      exitStatus = runCmd(postUpgradeCmd);
      checkCmdStatus(postUpgradeCmd, exitStatus);
    }
  } else if (action == "read" && !readFwCmd.empty()) {
    if (fwConfig.preUpgradeCmd().has_value()) {
      doPreUpgrade(fpd, *fwConfig.preUpgradeCmd());
    }
    XLOG(INFO) << "running readFwCmd";
    exitStatus = runCmd(readFwCmd);
    checkCmdStatus(readFwCmd, exitStatus);
  } else if (action == "verify" && !verifyFwCmd.empty()) {
    if (fwConfig.preUpgradeCmd().has_value()) {
      doPreUpgrade(fpd, *fwConfig.preUpgradeCmd());
    }
    exitStatus = runCmd(verifyFwCmd);
    checkCmdStatus(verifyFwCmd, exitStatus);
  } else {
    XLOG(INFO) << action << " not supported for " << fpd;
  }
}

void FwUtilImpl::storeFilePath(
    const std::string& fpd,
    const std::string& filePath) {
  const std::string textFile = "/home/" + fpd + "_filename.txt";
  const std::string cmd = "echo " + filePath + " > " + textFile;
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error("Run" + cmd + " failed!");
  }
}
void FwUtilImpl::removeFilePath(const std::string& fpd) {
  const std::string textFile = "/home/" + fpd + "_filename.txt";
  const std::string cmd = "rm " + textFile;
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error("Run" + cmd + " failed!");
  }
}
} // namespace facebook::fboss::platform::fw_util
