// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;

namespace {
// TODO: Move this mapping generation to cfgr
static BspPlatformMappingThrift buildMeru400bfuPlatformMapping() {
  BspPlatformMappingThrift mapping;
  BspPimMapping pimMapping;
  pimMapping.pimID() = 1;

  for (int tcvr = 1; tcvr <= 48; tcvr++) {
    BspTransceiverMapping tcvrMapping;
    tcvrMapping.tcvrId() = tcvr;

    BspResetPinInfo resetInfo;
    BspPresencePinInfo presenceInfo;
    std::string resetPath = "";
    std::string presencePath = "";
    if (tcvr <= 12) {
      presencePath = "/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_status_";
      resetPath = "/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_config_";
    } else if (tcvr <= 24) {
      presencePath = "/sys/bus/i2c/devices/2-0033/cpld_qsfpdd_port_status_";
      resetPath = "/sys/bus/i2c/devices/2-0033/cpld_qsfpdd_port_config_";
    } else if (tcvr <= 36) {
      presencePath = "/sys/bus/i2c/devices/2-0030/cpld_qsfpdd_port_status_";
      resetPath = "/sys/bus/i2c/devices/2-0030/cpld_qsfpdd_port_config_";
    } else {
      presencePath = "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_port_status_";
      resetPath = "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_port_config_";
    }
    resetPath += std::to_string((tcvr - 1) % 12);
    presencePath += std::to_string((tcvr - 1) % 12);
    resetInfo.sysfsPath() = resetPath;
    presenceInfo.sysfsPath() = presencePath;
    resetInfo.mask() = 0x1;
    presenceInfo.mask() = 0x2;
    BspTransceiverAccessControllerInfo accessControl;
    accessControl.controllerId() = fmt::format("accessController-{}", tcvr);
    accessControl.type() = ResetAndPresenceAccessType::CPLD;
    accessControl.reset() = resetInfo;
    accessControl.presence() = presenceInfo;
    tcvrMapping.accessControl() = accessControl;

    BspTransceiverIOControllerInfo tcvrIO;
    tcvrIO.controllerId() = fmt::format("ioController-{}", tcvr);
    tcvrIO.type() = TransceiverIOType::I2C;
    tcvrIO.devicePath() = "/dev/i2c-" + std::to_string(tcvr + 20);
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
Meru400bfuBspPlatformMapping::Meru400bfuBspPlatformMapping()
    : BspPlatformMapping(buildMeru400bfuPlatformMapping()) {}

} // namespace fboss
} // namespace facebook
