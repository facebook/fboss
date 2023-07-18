// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PkgUtils.h"

#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/Utils.h"

namespace facebook::fboss::platform::platform_manager {

void PkgUtils::run(const PlatformConfig& config) {
  XLOG(INFO) << "Installing BSP Kmods";
  int exitStatus{0};
  auto cmd = fmt::format(
      "dnf install {}-{} --assumeyes",
      *config.bspKmodsRpmName(),
      *config.bspKmodsRpmVersion());
  XLOG(INFO) << "Running command: " << cmd;
  XLOG(INFO) << helpers::execCommandUnchecked(cmd, exitStatus);
  if (exitStatus != 0) {
    throw std::runtime_error(
        fmt::format("Command failed. ExitStatus {}", exitStatus));
  }

  XLOG(INFO) << "Installing udev rules";
  cmd = fmt::format(
      "dnf install {}-{} --assumeyes",
      *config.udevRpmName(),
      *config.udevRpmVersion());
  XLOG(INFO) << "Running command: " << cmd;
  XLOG(INFO) << helpers::execCommandUnchecked(cmd, exitStatus);
  if (exitStatus != 0) {
    throw std::runtime_error(
        fmt::format("Command failed. ExitStatus {}", exitStatus));
  }
}

} // namespace facebook::fboss::platform::platform_manager
