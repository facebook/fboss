#pragma once

#include <set>
#include <string>

namespace facebook::fboss::platform::bsp_tests {

class WatchdogUtils {
 public:
  static std::set<std::string> getWatchdogs();
  static int getWatchdogTimeout(int fd);
  static void setWatchdogTimeout(int fd, int timeout);
  static void pingWatchdog(int fd);
  static void magicCloseWatchdog(int fd);
};

} // namespace facebook::fboss::platform::bsp_tests
