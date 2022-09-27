// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverCpldAccess.h"

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
#include "fboss/lib/bsp/BspTransceiverAccess.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspTransceiverCpldAccess::BspTransceiverCpldAccess(
    uint32_t tcvr,
    BspTransceiverMapping& tcvrMapping)
    : BspTransceiverAccessImpl(tcvr, tcvrMapping) {}

void BspTransceiverCpldAccess::init(bool forceReset) {
  CHECK(tcvrMapping_.accessControl()->reset()->sysfsPath());
  CHECK(tcvrMapping_.accessControl()->reset()->mask());
  auto resetPath = *tcvrMapping_.accessControl()->reset()->sysfsPath();
  uint8_t resetMask =
      static_cast<uint8_t>(*tcvrMapping_.accessControl()->reset()->mask());
  try {
    auto status = std::stoi(readSysfs(resetPath), nullptr, 16);
    if (forceReset) {
      status = status & ~resetMask;
      writeSysfs(resetPath, std::to_string(status));
    }
    status = status | resetMask;
    writeSysfs(resetPath, std::to_string(status));
  } catch (std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "BspTransceiverCpldAccessTrace: init() failed to reset/unreset TCVR {:d} (1 base)",
        tcvrID_);
  }
}

bool BspTransceiverCpldAccess::isPresent() {
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
        "BspTransceiverCpldAccessTrace: isPresent() failed to get Present status for TCVR {:d} (1 base)",
        tcvrID_);
  }
  return retVal;
}

} // namespace fboss
} // namespace facebook
