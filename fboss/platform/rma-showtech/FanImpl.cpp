// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/rma-showtech/FanImpl.h"

#include <gpiod.h>
#include <fstream>
#include <stdexcept>
#include <string>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/GpiodLine.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform {

FanImpl::FanImpl(const fan_service::Fan& fan) : fan_(fan) {}

bool FanImpl::readFanIsPresentOnDevice() const {
  unsigned int readVal{0};
  if (fan_.presenceSysfsPath()) {
    readVal =
        static_cast<unsigned>(readSysfsAsFloat(*fan_.presenceSysfsPath()));
    return readVal == *fan_.fanPresentVal();
  } else if (fan_.presenceGpio()) {
    struct gpiod_chip* chip =
        gpiod_chip_open(fan_.presenceGpio()->path()->c_str());
    readVal = GpiodLine(chip, *fan_.presenceGpio()->lineIndex(), "gpioline")
                  .getValue();
    return readVal == *fan_.presenceGpio()->desiredValue();
  } else {
    throw std::runtime_error("No fan presence config provided");
  }
}

int FanImpl::readFanRpm() const {
  return readSysfsAsFloat(*fan_.rpmSysfsPath());
}

int FanImpl::readFanPwmPercent() const {
  int fanPwm = readSysfsAsFloat(*fan_.pwmSysfsPath());
  return 100 * fanPwm / *fan_.pwmMax();
}

float FanImpl::readSysfsAsFloat(const std::string& path) {
  std::string buf = readSysfs(path);
  return std::stof(buf);
}

} // namespace facebook::fboss::platform
