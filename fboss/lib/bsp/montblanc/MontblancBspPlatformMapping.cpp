// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;

namespace {

constexpr int kMontblancNumTransceivers = 64;

static BspPlatformMappingThrift buildMontblancPlatformMapping() {
  BspPlatformMappingThrift mapping;
  BspPimMapping pimMapping;
  pimMapping.pimID() = 1;

  for (int tcvr = 1; tcvr <= kMontblancNumTransceivers; tcvr++) {
    BspTransceiverMapping tcvrMapping;
    tcvrMapping.tcvrId() = tcvr;

    BspResetPinInfo resetInfo;
    BspPresencePinInfo presenceInfo;
    // TODO(rajank): Correct these sysfs path
    std::string presencePath = "/sys/bus/i2c/devices/" + std::to_string(tcvr) +
        "/cpld_qsfpdd_intr_present_0";
    std::string resetPath = "/sys/bus/i2c/devices/" + std::to_string(tcvr) +
        "/cpld_qsfpdd_intr_reset_0";
    int accessMask = 0x01;

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
    std::string devPath =
        "/run/devmap/i2c-buses/OPTICS_" + std::to_string(tcvr);

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

MontblancBspPlatformMapping::MontblancBspPlatformMapping()
    : BspPlatformMapping(buildMontblancPlatformMapping()) {}

} // namespace fboss
} // namespace facebook
