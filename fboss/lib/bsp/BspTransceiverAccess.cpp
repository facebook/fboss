// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverAccess.h"

#include <fcntl.h>
#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspTransceiverAccess::BspTransceiverAccess(
    uint32_t tcvr,
    BspTransceiverMapping& tcvrMapping) {
  tcvrMapping_ = tcvrMapping;
  tcvrID_ = tcvr;
}

void BspTransceiverAccess::init(bool forceReset) {
  // TODO: Check access type (GPIO/CPLD) and handle accordingly
  CHECK(tcvrMapping_.accessControl()->reset()->sysfsPath());
  CHECK(tcvrMapping_.accessControl()->reset()->mask());
  auto resetPath = *tcvrMapping_.accessControl()->reset()->sysfsPath();
  auto resetMask = *tcvrMapping_.accessControl()->reset()->mask();
  try {
    auto status = std::stoi(readSysfs(resetPath), nullptr, 16);
    if (forceReset) {
      status = status & ~(1 << resetMask);
      writeSysfs(resetPath, std::to_string(status));
    }
    status = status | resetMask;
    writeSysfs(resetPath, std::to_string(status));
  } catch (std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "BspTransceiverIOTrace: init() failed to reset/unreset TCVR {:d} (1 base)",
        tcvrID_);
  }
}

bool BspTransceiverAccess::isPresent() {
  // TODO: Check access type (GPIO/CPLD) and handle accordingly
  bool retVal = false;
  CHECK(tcvrMapping_.accessControl()->presence()->sysfsPath());
  CHECK(tcvrMapping_.accessControl()->presence()->mask());
  auto presencePath = *tcvrMapping_.accessControl()->presence()->sysfsPath();
  auto presenceMask = *tcvrMapping_.accessControl()->presence()->mask();
  try {
    auto status = std::stoi(readSysfs(presencePath), nullptr, 16);
    retVal = !(status & presenceMask);
  } catch (std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "BspTransceiverIOTrace: isPresent() failed to get Present status for TCVR {:d} (1 base)",
        tcvrID_);
  }
  return retVal;
}

} // namespace fboss
} // namespace facebook
