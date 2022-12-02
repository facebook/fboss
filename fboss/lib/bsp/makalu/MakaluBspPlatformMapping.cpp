// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/makalu/MakaluBspPlatformMapping.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;

namespace {
// TODO: Move this mapping generation to cfgr
static BspPlatformMappingThrift buildMakaluPlatformMapping() {
  BspPlatformMappingThrift mapping;
  BspPimMapping pimMapping;
  pimMapping.pimID() = 1;

  for (int tcvr = 1; tcvr <= 76; tcvr++) {
    BspTransceiverMapping tcvrMapping;
    tcvrMapping.tcvrId() = tcvr;

    BspResetPinInfo resetInfo;
    BspPresencePinInfo presenceInfo;
    std::string resetPath = "";
    std::string presencePath = "";
    int accessMask = 0;
    if (tcvr <= 8) {
      presencePath = "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0";
      resetPath = "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0";
      accessMask = 1 << ((tcvr - 1) % 8);
    } else if (tcvr <= 16) {
      presencePath = "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1";
      resetPath = "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1";
      accessMask = 1 << ((tcvr - 1) % 8);
    } else if (tcvr <= 18) {
      presencePath = "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_2";
      resetPath = "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_2";
      accessMask = 1 << ((tcvr - 1) % 8);
    } else if (tcvr <= 26) {
      presencePath = "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0";
      resetPath = "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0";
      accessMask = 1 << ((tcvr - 19) % 8);
    } else if (tcvr <= 34) {
      presencePath = "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1";
      resetPath = "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1";
      accessMask = 1 << ((tcvr - 19) % 8);
    } else if (tcvr <= 36) {
      presencePath = "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_2";
      resetPath = "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_2";
      accessMask = 1 << ((tcvr - 19) % 8);
    } else if (tcvr <= 44) {
      presencePath = "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0";
      resetPath = "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0";
      accessMask = 1 << ((tcvr - 37) % 8);
    } else if (tcvr <= 52) {
      presencePath = "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1";
      resetPath = "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1";
      accessMask = 1 << ((tcvr - 37) % 8);
    } else if (tcvr <= 56) {
      presencePath = "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_2";
      resetPath = "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_2";
      accessMask = 1 << ((tcvr - 37) % 8);
    } else if (tcvr <= 64) {
      presencePath = "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0";
      resetPath = "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0";
      accessMask = 1 << ((tcvr - 57) % 8);
    } else if (tcvr <= 72) {
      presencePath = "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1";
      resetPath = "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1";
      accessMask = 1 << ((tcvr - 57) % 8);
    } else {
      presencePath = "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_2";
      resetPath = "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_2";
      accessMask = 1 << ((tcvr - 57) % 8);
    }
    resetInfo.sysfsPath() = resetPath;
    presenceInfo.sysfsPath() = presencePath;
    resetInfo.mask() = accessMask;
    presenceInfo.mask() = accessMask;
    BspTransceiverAccessControllerInfo accessControl;
    accessControl.controllerId() = fmt::format("accessController-{}", tcvr);
    accessControl.type() = ResetAndPresenceAccessType::CPLD;
    accessControl.reset() = resetInfo;
    accessControl.presence() = presenceInfo;
    tcvrMapping.accessControl() = accessControl;

    BspTransceiverIOControllerInfo tcvrIO;
    tcvrIO.controllerId() = fmt::format("ioController-{}", tcvr);
    tcvrIO.type() = TransceiverIOType::I2C;
    std::string devPath = "";
    if (tcvr <= 36) {
      devPath = "/dev/i2c-" + std::to_string(tcvr + 72);
    } else {
      devPath = "/dev/i2c-" + std::to_string(tcvr - 37 + 33);
    }
    tcvrIO.devicePath() = devPath;
    tcvrMapping.io() = tcvrIO;

    pimMapping.tcvrMapping()[tcvr] = tcvrMapping;
  }
  mapping.pimMapping()[1] = pimMapping;
  return mapping;
}

} // namespace

namespace facebook {
namespace fboss {

// TODO: Use pre generated bsp platform mapping from cfgr
MakaluBspPlatformMapping::MakaluBspPlatformMapping()
    : BspPlatformMapping(buildMakaluPlatformMapping()) {}

} // namespace fboss
} // namespace facebook
