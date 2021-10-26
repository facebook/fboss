// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sysexits.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include "fboss/platform/helpers/ScdHelper.h"
#include "fboss/platform/helpers/utils.h"

using namespace facebook::fboss::platform::helpers;

DEFINE_int32(timeout, 300, "Timeout value in seconds");

DEFINE_bool(disable, false, "Disable watchdog");

DEFINE_bool(h, false, "Help");

void print_usage(void) {
  std::cout
      << "Watchdog service, kick it every 15s, expire in 3 minutes (defaul) if not kicked"
      << std::endl;
  std::cout << "usage examples:" << std::endl;
  std::cout << "Set timeout to 200s: wdt_service --timeout 200" << std::endl;
  std::cout << "Use default 300s timeout: wdt_service" << std::endl;
  std::cout << "To disable watchdog: wdt_service --disable";
}

/*
 * This utility program will output Chassis info for Darwin
 */
int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  if (FLAGS_h) {
    print_usage();
    return EX_OK;
  }

  if (FLAGS_disable) {
    auto scd = ScdHelper();
    scd.disable_wdt();
    return EX_OK;
  }

  if (FLAGS_timeout < 60 || FLAGS_timeout > 655) {
    LOG(INFO) << "Timeout value must be between 30 seconds and 655 seconds";
    return EX_OK;
  }

  while (true) {
    auto scd = ScdHelper();

    scd.kick_wdt(FLAGS_timeout);

    /* sleep override */
    std::this_thread::sleep_for(std::chrono::seconds(15));
    LOG(INFO) << "watchdog kicked";
  }

  LOG(INFO) << "Exit wdt_service";
  return EX_OK;
}
