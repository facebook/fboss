// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"

#include <sys/stat.h>
#include <fcntl.h>

#include <folly/FileUtil.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>

using std::string;

DEFINE_bool(can_warm_boot, true,
            "Enable/disable warm boot functionality");
DEFINE_string(switch_state_file, "switch_state",
    "File for dumping switch state JSON in on exit");

namespace {
constexpr auto wbFlagPrefix = "can_warm_boot_";
constexpr auto wbDataPrefix = "bcm_sdk_state_";
constexpr auto forceColdBootPrefix = "cold_boot_once_";
constexpr auto shutdownDumpPrefix = "sdk_shutdown_dump_";
constexpr auto startupDumpPrefix = "sdk_startup_dump_";

/*
 * Remove the given file. Return true if file exists and
 * we were able to remove it, false otherwise
 */
bool removeFile(const string& filename) {
  int rv = unlink(filename.c_str());
  if (rv == 0) {
    // The file existed and we successfully removed it.
    return true;
  }
  if (errno == ENOENT) {
    // The file wasn't present.
    return false;
  }
  // Some other unexpected error.
  throw facebook::fboss::SysError(
      errno, "error while trying to remove warm boot file ", filename);
}

}

namespace facebook { namespace fboss {

DiscBackedBcmWarmBootHelper::DiscBackedBcmWarmBootHelper(
    int unit,
    std::string warmBootDir)
    : unit_(unit), warmBootDir_(warmBootDir) {
  if (!warmBootDir_.empty()) {
    // Make sure the warm boot directory exists.
    //
    // TODO(aeckert): Creating the dir probably belongs somewhere else, possibly
    // in BcmPlatform? Putting it here for now so we encapsulate as much
    // of the warm boot related stuff as possible in this class.
    utilCreateDir(warmBootDir_);

    canWarmBoot_ = checkAndClearWarmBootFlags();
    if (!FLAGS_can_warm_boot) {
      canWarmBoot_ = false;
    }

    auto bootType = canWarmBoot_ ? "WARM" : "COLD";
    XLOG(DBG1) << "Will attempt " << bootType << " boot";

    setupWarmBootFile();
  }
}

DiscBackedBcmWarmBootHelper::~DiscBackedBcmWarmBootHelper() {
  if (warmBootFd_ > 0) {
    int rv = close(warmBootFd_);
    if (rv < 0) {
      XLOG(ERR) << "error closing warm boot file for unit " << unit_ << ": "
                << errno;
    }
    warmBootFd_ = -1;
  }
}

std::string DiscBackedBcmWarmBootHelper::warmBootSwitchStateFile() const {
  return folly::to<string>(warmBootDir_, "/", FLAGS_switch_state_file);
}

std::string DiscBackedBcmWarmBootHelper::warmBootFlag() const {
  return folly::to<string>(warmBootDir_, "/", wbFlagPrefix, unit_);
}

std::string DiscBackedBcmWarmBootHelper::warmBootDataPath() const {
  return folly::to<string>(warmBootDir_, "/", wbDataPrefix, unit_);
}

std::string DiscBackedBcmWarmBootHelper::forceColdBootOnceFlag() const {
  return folly::to<string>(warmBootDir_, "/", forceColdBootPrefix, unit_);
}

std::string DiscBackedBcmWarmBootHelper::startupSdkDumpFile() const {
  return folly::to<string>(warmBootDir_, "/", startupDumpPrefix, unit_);
}

std::string DiscBackedBcmWarmBootHelper::shutdownSdkDumpFile() const {
  return folly::to<string>(warmBootDir_, "/", shutdownDumpPrefix, unit_);
}

void DiscBackedBcmWarmBootHelper::setCanWarmBoot() {
  auto wbFlag = warmBootFlag();
  auto updateFd = creat(wbFlag.c_str(),
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (updateFd < 0) {
    throw SysError(errno, "Unable to create ", wbFlag);
  }
  close(updateFd);

  XLOG(DBG1) << "Wrote can warm boot flag: " << wbFlag;
}

bool DiscBackedBcmWarmBootHelper::checkAndClearWarmBootFlags() {
  // Return true if coldBootOnceFile does not exist and
  // canWarmBoot file exists
  bool canWarmBoot = removeFile(warmBootFlag());
  bool forceColdBoot = removeFile(forceColdBootOnceFlag());
  return !forceColdBoot && canWarmBoot;
}

bool DiscBackedBcmWarmBootHelper::storeWarmBootState(
    const folly::dynamic& switchState) {
  return dumpStateToFile(warmBootSwitchStateFile(), switchState);
}

folly::dynamic DiscBackedBcmWarmBootHelper::getWarmBootState() const {
  std::string warmBootJson;
  auto ret = folly::readFile(warmBootSwitchStateFile().c_str(), warmBootJson);
  sysCheckError(
      ret, "Unable to read switch state from : ", warmBootSwitchStateFile());
  return folly::parseJson(warmBootJson);
}

} // namespace fboss
} // namespace facebook
