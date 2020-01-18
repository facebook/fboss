// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFacebookAPI.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <chrono>

#include <folly/FileUtil.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>

extern "C" {
#include <bcm/switch.h>
#if (!defined(BCM_VER_MAJOR))
#include <soc/opensoc.h>
#else
#include <soc/scache.h>
#endif
} // extern "C"

using std::string;

using facebook::fboss::BcmAPI;

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;

DEFINE_bool(can_warm_boot, true, "Enable/disable warm boot functionality");
DEFINE_string(
    switch_state_file,
    "switch_state",
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

} // namespace

namespace facebook::fboss {

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
  auto updateFd = creat(wbFlag.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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
  warmBootStateWritten_ =
      dumpStateToFile(warmBootSwitchStateFile(), switchState);
  return warmBootStateWritten_;
}

folly::dynamic DiscBackedBcmWarmBootHelper::getWarmBootState() const {
  std::string warmBootJson;
  auto ret = folly::readFile(warmBootSwitchStateFile().c_str(), warmBootJson);
  sysCheckError(
      ret, "Unable to read switch state from : ", warmBootSwitchStateFile());
  return folly::parseJson(warmBootJson);
}

int BcmWarmBootHelper::warmBootReadCallback(
    int unitNumber,
    uint8_t* buf,
    int offset,
    int nbytes) {
  try {
    auto wbHelper = BcmAPI::getUnit(unitNumber)->warmBootHelper();
    wbHelper->warmBootRead(buf, offset, nbytes);
    return nbytes;
  } catch (const std::exception& ex) {
    XLOG(ERR) << "error performing warm boot read of " << nbytes
              << " bytes for unit " << unitNumber << ": "
              << folly::exceptionStr(ex);
    return BCM_E_FAIL;
  }
}

int BcmWarmBootHelper::warmBootWriteCallback(
    int unitNumber,
    uint8_t* buf,
    int offset,
    int nbytes) {
  try {
    auto wbHelper = BcmAPI::getUnit(unitNumber)->warmBootHelper();
    wbHelper->warmBootWrite(buf, offset, nbytes);
    return nbytes;
  } catch (const std::exception& ex) {
    XLOG(ERR) << "error performing warm boot write of " << nbytes
              << " bytes for unit " << unitNumber << ": "
              << folly::exceptionStr(ex);
    // Ugh.  Unfortunately the Broadcom SDK code doesn't appear to
    // check the return value from the warm boot write callback, so
    // it won't correctly handle errors at this point.
    return BCM_E_FAIL;
  }
}

void DiscBackedBcmWarmBootHelper::setupWarmBootFile() {
  auto warmBootPath = warmBootDataPath();
  warmBootFd_ = open(warmBootPath.c_str(), O_RDWR | O_CREAT, 0600);
  if (warmBootFd_ < 0) {
    throw SysError(errno, "failed to open warm boot data file ", warmBootPath);
  }

  auto rv = soc_stable_set(unit_, BCM_SWITCH_STABLE_APPLICATION, 0);
  bcmCheckError(rv, "unable to configure for warm boot for unit ", unit_);

  rv = soc_switch_stable_register(
      unit_, &warmBootReadCallback, &warmBootWriteCallback, nullptr, nullptr);
  bcmCheckError(
      rv,
      "unable to register read, write callbacks for warm boot "
      "on unit ",
      unit_);

  auto configStableSize = BcmAPI::getConfigValue("stable_size");
  auto stableSize = (configStableSize)
      ? std::stoul(configStableSize, nullptr, 0)
      : SOC_DEFAULT_LVL2_STABLE_SIZE;
// Remove this code once the new values for stable are propogated
// to all switches
#define STABLE_SIZE_MINIMUM 0x6000000
  if (stableSize < STABLE_SIZE_MINIMUM) {
    stableSize = STABLE_SIZE_MINIMUM;
  }
  XLOG(DBG0) << "Initializing sdk stable storage with max size of "
             << stableSize << " bytes.";
  rv = soc_stable_size_set(unit_, stableSize);
  bcmCheckError(
      rv, "unable to set size for warm boot storage for unit ", unit_);
}

void DiscBackedBcmWarmBootHelper::warmBootRead(
    uint8_t* buf,
    int offset,
    int nbytes) {
  if (warmBootFd_ < 0) {
    // This shouldn't ever happen.  We only register the warm boot
    // callbacks after opening the fd.
    throw FbossError(
        "attempted warm boot read on unit ",
        unit_,
        " but warm boot not configured");
  }

  // The Broadcom code assumes that the read callback always returns
  // exactly as much data as requested, so use folly::preadFull().
  // This should always return the full amount read, or an error.
  auto bytesRead = folly::preadFull(warmBootFd_, buf, nbytes, offset);
  if (bytesRead < 0) {
    throw SysError(
        errno,
        "error reading ",
        nbytes,
        " bytes from warm boot file for unit ",
        unit_);
  }
}

void DiscBackedBcmWarmBootHelper::warmBootWrite(
    const uint8_t* buf,
    int offset,
    int nbytes) {
  if (warmBootFd_ < 0) {
    // This shouldn't ever happen.  We only register the warm boot
    // callbacks after opening the fd.
    throw FbossError(
        "attempted warm boot write on unit ",
        unit_,
        " but warm boot not configured");
  }

  auto bytesWritten = folly::pwriteFull(warmBootFd_, buf, nbytes, offset);
  if (bytesWritten < 0) {
    throw SysError(
        errno,
        "error writing ",
        nbytes,
        " bytes to warm boot file for unit ",
        unit_);
  }
}

} // namespace facebook::fboss
