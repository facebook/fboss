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

namespace facebook::fboss {

BcmWarmBootHelper::BcmWarmBootHelper(int unit, const std::string& warmBootDir)
    : HwSwitchWarmBootHelper(unit, warmBootDir, "bcm_sdk_state_") {
  if (!warmBootDir.empty()) {
    setupSdkWarmBoot();
  }
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

void BcmWarmBootHelper::setupSdkWarmBoot() {
  auto rv = soc_stable_set(getSwitchId(), BCM_SWITCH_STABLE_APPLICATION, 0);
  bcmCheckError(
      rv, "unable to configure for warm boot for unit ", getSwitchId());

  rv = soc_switch_stable_register(
      getSwitchId(),
      &warmBootReadCallback,
      &warmBootWriteCallback,
      nullptr,
      nullptr);
  bcmCheckError(
      rv,
      "unable to register read, write callbacks for warm boot "
      "on unit ",
      getSwitchId());

  auto configStableSize = BcmAPI::getConfigValue("stable_size");
  auto stableSize = (configStableSize)
      ? std::stoul(configStableSize, nullptr, 0)
      : SOC_DEFAULT_LVL2_STABLE_SIZE;
  // Remove this code once the new values for stable are propagated
  // to all switches
  constexpr uint64_t kStableSizeMinimum = 0x6000000;
  if (stableSize < kStableSizeMinimum) {
    stableSize = kStableSizeMinimum;
  }
  XLOG(DBG0) << "Initializing sdk stable storage with max size of "
             << stableSize << " bytes.";
  rv = soc_stable_size_set(getSwitchId(), stableSize);
  bcmCheckError(
      rv, "unable to set size for warm boot storage for unit ", getSwitchId());
}

void BcmWarmBootHelper::warmBootRead(uint8_t* buf, int offset, int nbytes) {
  if (warmBootFd() < 0) {
    // This shouldn't ever happen.  We only register the warm boot
    // callbacks after opening the fd.
    throw FbossError(
        "attempted warm boot read on unit ",
        getSwitchId(),
        " but warm boot not configured");
  }

  // The Broadcom code assumes that the read callback always returns
  // exactly as much data as requested, so use folly::preadFull().
  // This should always return the full amount read, or an error.
  auto bytesRead = folly::preadFull(warmBootFd(), buf, nbytes, offset);
  if (bytesRead < 0) {
    throw SysError(
        errno,
        "error reading ",
        nbytes,
        " bytes from warm boot file for unit ",
        getSwitchId());
  }
}

void BcmWarmBootHelper::warmBootWrite(
    const uint8_t* buf,
    int offset,
    int nbytes) {
  if (warmBootFd() < 0) {
    // This shouldn't ever happen.  We only register the warm boot
    // callbacks after opening the fd.
    throw FbossError(
        "attempted warm boot write on unit ",
        getSwitchId(),
        " but warm boot not configured");
  }

  auto bytesWritten = folly::pwriteFull(warmBootFd(), buf, nbytes, offset);
  if (bytesWritten < 0) {
    throw SysError(
        errno,
        "error writing ",
        nbytes,
        " bytes to warm boot file for unit ",
        getSwitchId());
  }
}

} // namespace facebook::fboss
