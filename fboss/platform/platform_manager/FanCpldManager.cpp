// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/FanCpldManager.h"

#include <fcntl.h>
#include <sys/ioctl.h>

#include <filesystem>

#include <fmt/format.h>
#include <folly/File.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/Utils.h"
#include "fboss/platform/platform_manager/uapi/fbfancpld-ioctl.h"

namespace facebook::fboss::platform::platform_manager {
namespace {

constexpr auto kCharDevWaitSecs = std::chrono::seconds(5);

std::string getFanCpldCharDevPath(uint16_t busNum, const I2cAddr& addr) {
  return fmt::format("/dev/fbfancpld-{}-{}", busNum, addr.hex4Str());
}

} // namespace

void configureFanCpld(
    uint16_t busNum,
    const I2cAddr& addr,
    const FanCpldConfig& fanCpldConfig) {
  auto charDevPath = getFanCpldCharDevPath(busNum, addr);

  if (!Utils().checkDeviceReadiness(
          [&]() -> bool { return std::filesystem::exists(charDevPath); },
          fmt::format(
              "Fan CPLD char dev {} is not yet created. Waiting for at most {}s",
              charDevPath,
              kCharDevWaitSecs.count()),
          kCharDevWaitSecs)) {
    throw std::runtime_error(
        fmt::format(
            "Failed to configure fan CPLD: char dev {} not found",
            charDevPath));
  }

  XLOG(INFO) << fmt::format("Configuring fan CPLD on {}", charDevPath);

  folly::File devFile;
  try {
    devFile = folly::File(charDevPath.c_str(), O_RDWR);
  } catch (const std::system_error& e) {
    throw std::runtime_error(
        fmt::format(
            "Failed to open fan CPLD char dev {}: {}", charDevPath, e.what()));
  }

  struct fbfancpld_ioctl_config config{};
  config.num_fans = *fanCpldConfig.numFans();
  config.pwm_max = *fanCpldConfig.pwmMax();
  config.speed_multiplier = *fanCpldConfig.speedMultiplier();
  config.has_rear_tach = *fanCpldConfig.hasRearTach() ? 1 : 0;
  config.has_leds = *fanCpldConfig.hasLeds() ? 1 : 0;

  int ret = ioctl(devFile.fd(), FBFANCPLD_IOC_CONFIGURE, &config);
  if (ret < 0) {
    if (errno == EBUSY) {
      XLOG(INFO) << fmt::format(
          "Fan CPLD on {} is already configured, skipping", charDevPath);
      return;
    }
    throw std::runtime_error(
        fmt::format(
            "Failed to configure fan CPLD on {}: ioctl failed: {}",
            charDevPath,
            folly::errnoStr(errno)));
  }

  XLOG(INFO) << fmt::format(
      "Configured fan CPLD on {}: fans={} pwm_max={} speed_mult={} "
      "rear_tach={} leds={}",
      charDevPath,
      *fanCpldConfig.numFans(),
      *fanCpldConfig.pwmMax(),
      *fanCpldConfig.speedMultiplier(),
      *fanCpldConfig.hasRearTach(),
      *fanCpldConfig.hasLeds());
}

} // namespace facebook::fboss::platform::platform_manager
