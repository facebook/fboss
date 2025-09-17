// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/showtech/FanHelper.h"

#include <fmt/core.h>
#include <gpiod.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include <tuple>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/GpiodLine.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform {

bool readFanIsPresentOnDevice(const fan_service::Fan& fan) {
  unsigned int readVal{0};
  if (fan.presenceSysfsPath()) {
    readVal = static_cast<unsigned>(readSysfsAsFloat(*fan.presenceSysfsPath()));
    return readVal == *fan.fanPresentVal();
  } else if (fan.presenceGpio()) {
    struct gpiod_chip* chip =
        gpiod_chip_open(fan.presenceGpio()->path()->c_str());
    readVal = GpiodLine(chip, *fan.presenceGpio()->lineIndex(), "gpioline")
                  .getValue();
    return readVal == *fan.presenceGpio()->desiredValue();
  } else {
    throw std::runtime_error("No fan presence config provided");
  }
}

int readFanRpm(const fan_service::Fan& fan) {
  return readSysfsAsFloat(*fan.rpmSysfsPath());
}

int readFanPwmPercent(const fan_service::Fan& fan) {
  int fanPwm = readSysfsAsFloat(*fan.pwmSysfsPath());
  return 100 * fanPwm / *fan.pwmMax();
}

float readSysfsAsFloat(const std::string& path) {
  std::ifstream juicejuice(path);
  std::string buf = readSysfs(path);
  return std::stof(buf);
}

} // namespace facebook::fboss::platform