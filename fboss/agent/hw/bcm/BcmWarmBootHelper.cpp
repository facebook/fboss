// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glog/logging.h>

using std::string;

DEFINE_bool(can_warm_boot, true,
            "Enable/disable warm boot functionality");

namespace {
constexpr auto wbFlagPrefix = "can_warm_boot_";
constexpr auto wbDataPrefix = "bcm_sdk_state_";
constexpr auto forceColdBootPrefix = "cold_boot_once_";

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

BcmWarmBootHelper::BcmWarmBootHelper(int unit, std::string warmBootDir)
    : unit_(unit),
      warmBootDir_(warmBootDir) {
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
    VLOG(1) << "Will attempt " << bootType << " boot";

    setupWarmBootFile();
  }
}

BcmWarmBootHelper::~BcmWarmBootHelper() {
  if (warmBootFd_ > 0) {
    int rv = close(warmBootFd_);
    if (rv < 0) {
      LOG(ERROR) << "error closing warm boot file for unit " << unit_
        << ": " << errno;
    }
    warmBootFd_ = -1;
  }
}

std::string BcmWarmBootHelper::warmBootFlag() const {
  return folly::to<string>(warmBootDir_, "/", wbFlagPrefix, unit_);
}

std::string BcmWarmBootHelper::warmBootDataPath() const {
  return folly::to<string>(warmBootDir_, "/", wbDataPrefix, unit_);
}

std::string BcmWarmBootHelper::forceColdBootOnceFlag() const {
  return folly::to<string>(warmBootDir_, "/", forceColdBootPrefix, unit_);
}

bool BcmWarmBootHelper::canWarmBoot() {
  return canWarmBoot_;
}

void BcmWarmBootHelper::setCanWarmBoot() {
  auto wbFlag = warmBootFlag();
  auto updateFd = creat(wbFlag.c_str(),
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (updateFd < 0) {
    throw SysError(errno, "Unable to create ", wbFlag);
  }
  close(updateFd);

  VLOG(1) << "Wrote can warm boot flag: " << wbFlag;
}

bool BcmWarmBootHelper::checkAndClearWarmBootFlags() {
  // Return true if coldBootOnceFile does not exist and
  // canWarmBoot file exists
  bool canWarmBoot = removeFile(warmBootFlag());
  bool forceColdBoot = removeFile(forceColdBootOnceFlag());
  return !forceColdBoot && canWarmBoot;
}

}} // facebook::fboss
