// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PkgUtils.h"

#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::platform_manager {

void PkgUtils::run(const PlatformConfig& config) {
  XLOG(INFO) << "Installing BSP Kmods";
  runImpl(
      fmt::format(
          "{}-{}", *config.bspKmodsRpmName(), *config.bspKmodsRpmVersion()),
      3 /* maxAttempts */);

  XLOG(INFO) << "Installing udev rules";
  runImpl(
      fmt::format("{}-{}", *config.udevRpmName(), *config.udevRpmVersion()),
      3 /* maxAttempts */);
}

void PkgUtils::runImpl(const std::string& rpmFullName, int maxAttempts) {
  int exitStatus{0}, attempt{1};
  std::string standardOut{};
  auto cmd = fmt::format("dnf install {} --assumeyes", rpmFullName);
  do {
    XLOG(INFO) << fmt::format(
        "Running command: `{}`; Attempt: {}", cmd, attempt++);
    std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(cmd);
    XLOG(INFO) << standardOut;
  } while (attempt <= maxAttempts && exitStatus != 0);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Command `{}` failed with exit code {}", cmd, exitStatus);
    throw std::runtime_error(fmt::format(
        "Failed to install rpm `{}` with exit code {}",
        rpmFullName,
        exitStatus));
  }
}

} // namespace facebook::fboss::platform::platform_manager
