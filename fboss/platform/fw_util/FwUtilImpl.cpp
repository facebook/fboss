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

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fw_util/Flags.h"
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/Utils.h"

using namespace folly::literals::shell_literals;
using namespace facebook::fboss::platform::helpers;
namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::init() {
  std::string fwUtilConfJson;
  // Check if conf file name is set, if not, set the default name
  if (confFileName_.empty()) {
    XLOG(INFO) << "No config file was provided. Inferring from config_lib";
    fwUtilConfJson = ConfigLib().getFwUtilConfig();
  } else {
    XLOG(INFO) << "Using config file: " << confFileName_;
    if (!folly::readFile(confFileName_.c_str(), fwUtilConfJson)) {
      throw std::runtime_error(
          "Can not find firmware config file: " + confFileName_);
    }
  }
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
  int retVal = 0;
  std::string version = execCommandUnchecked(cmd, retVal);
  if (retVal != 0) {
    throw std::runtime_error("Run " + cmd + " failed!");
  }

  return version;
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

void FwUtilImpl::doFirmwareAction(
    const std::string& fpd,
    const std::string& action) {
  int exitStatus = 0;
  auto iter = fwUtilConfig_.fwConfigs()->find(fpd);
  if (iter == fwUtilConfig_.fwConfigs()->end()) {
    XLOG(INFO)
        << " is not part of the firmware target_name list Please run ./fw-util --helpon=Flags for the right usage";
    return;
  }

  const std::string upgradeCmd = *iter->second.upgradeCmd();
  const std::string readFwCmd = *iter->second.readFwCmd();
  const std::string verifyFwCmd = *iter->second.verifyFwCmd();

  if (action == "program" && !upgradeCmd.empty()) {
    if (iter->second.preUpgradeCmd().has_value()) {
      doPreUpgrade(fpd, *iter->second.preUpgradeCmd());
    }
    XLOG(INFO) << "running upgradeCmd";
    exitStatus = runCmd(upgradeCmd);
    checkCmdStatus(upgradeCmd, exitStatus);
    if (iter->second.postUpgradeCmd().has_value()) {
      XLOG(INFO) << "running postUpgradeCmd";
      const std::string postUpgradeCmd = *iter->second.postUpgradeCmd();
      exitStatus = runCmd(postUpgradeCmd);
      checkCmdStatus(postUpgradeCmd, exitStatus);
    }
  } else if (action == "read" && !readFwCmd.empty()) {
    if (iter->second.preUpgradeCmd().has_value()) {
      doPreUpgrade(fpd, *iter->second.preUpgradeCmd());
    }
    XLOG(INFO) << "running readFwCmd";
    exitStatus = runCmd(readFwCmd);
    checkCmdStatus(readFwCmd, exitStatus);
  } else if (action == "verify" && !verifyFwCmd.empty()) {
    if (iter->second.preUpgradeCmd().has_value()) {
      doPreUpgrade(fpd, *iter->second.preUpgradeCmd());
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
  int exitStatus = 0;
  const std::string textFile = "/tmp/" + fpd + "_filename.txt";
  const std::string cmd = "echo " + filePath + " > " + textFile;
  execCommandUnchecked(cmd, exitStatus);
  if (exitStatus != 0) {
    throw std::runtime_error("Run" + cmd + " failed!");
  }
}
} // namespace facebook::fboss::platform::fw_util
