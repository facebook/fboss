#include "fboss/platform/bsp_tests/utils/WatchdogUtils.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <filesystem>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

namespace facebook::fboss::platform::bsp_tests {

// Watchdog IOCTL commands defined in:
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/watchdog.h
#define WATCHDOG_IOCTL_BASE 'W'

// IOR and IOWR macros are defined in <linux/ioctl.h>
#define WDIOC_KEEPALIVE _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define WDIOC_SETTIMEOUT _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_GETTIMEOUT _IOR(WATCHDOG_IOCTL_BASE, 7, int)

std::set<std::string> WatchdogUtils::getWatchdogs() {
  std::set<std::string> watchdogs;

  for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
    if (entry.path().filename().string().starts_with("watchdog")) {
      watchdogs.insert(entry.path().string());
    }
  }
  return watchdogs;
}

int WatchdogUtils::getWatchdogTimeout(int fd) {
  int timeout = 0;
  int result = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);

  if (result != 0) {
    throw std::runtime_error(
        fmt::format("Failed to get watchdog timeout: {}", result));
  }

  return timeout;
}

void WatchdogUtils::setWatchdogTimeout(int fd, int timeout) {
  int result = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);

  if (result != 0) {
    throw std::runtime_error(
        fmt::format("Failed to set watchdog timeout: {}", result));
  }
}

void WatchdogUtils::pingWatchdog(int fd) {
  int result = ioctl(fd, WDIOC_KEEPALIVE, 0);

  if (result != 0) {
    throw std::runtime_error(
        fmt::format("Failed to ping watchdog: {}", result));
  }
}

void WatchdogUtils::magicCloseWatchdog(int fd) {
  // Write "V" to the file descriptor to disarm the watchdog
  const char magic = 'V';
  ssize_t bytesWritten = write(fd, &magic, 1);

  if (bytesWritten != 1) {
    throw std::runtime_error(
        fmt::format(
            "Failed to write magic close character to watchdog: {}", errno));
  }
}

} // namespace facebook::fboss::platform::bsp_tests
