// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <string.h>
#include <sysexits.h>
#include <iostream>
#include <ratio>
#include "./helpers/utils.h"

using namespace facebook::fboss::platform::helpers;

DEFINE_bool(s, false, "Reset userver or chassis");
DEFINE_bool(status, false, "Show Power Status");
DEFINE_bool(h, false, "Help");
/*
 *Print Power Status, including CPU and Switch Card
 */

void print_status(void) {
  // To Do: add more status info
  std::cout << "Switch card pownered on" << std::endl;
}

void print_usage(void) {
  std::cout << "wedge_power usage:" << std::endl;
  std::cout << "To power cycle whole chasis: wedge_power reset -s" << std::endl;
  std::cout << "To reboot uServer: wedge_power reset" << std::endl;
}
void reboot_userver(void) {
  int out = 0;

  std::cout << "Power Cycle uServer/CPU board" << std::endl;

  std::string res = execCommand("sudo systemctl reboot", out);

  if (out != 0) {
    std::cout << "Reboot uServer failed!" << std::endl;
  }
}

void power_cycle_chassis(void) {
  LOG(INFO) << "Power Cycle Whole Chassis";
  // Rook CPLD, typically memory mapped at base offset 0xb0000000
  // Reg offset 7000 is for power cycling the whole chassis
  uint32_t val = 0xdead; // For whole chassis power cycle
  if (mmap_write(0xb0007000, 'w', val) != 0) {
    std::cout << "Failed to power cycle whole chassis" << std::endl;
  }
}

void power_reset(bool flag) {
  if (flag) {
    power_cycle_chassis();
  } else {
    reboot_userver();
  }
}

/*
 * This utility program provides power cycle service for darwin uServer and the
 * whole chassis
 */
int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  if (argc == 1 || FLAGS_h) {
    print_usage();
    return EX_OK;
  }

  if (FLAGS_status) {
    print_status();
    return EX_OK;
  }

  if (argc > 1 && (std::string(argv[1]) == std::string("reset"))) {
    power_reset(FLAGS_s);
    return EX_OK;
  }

  return EX_OK;
}
