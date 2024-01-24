// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <string.h>
#include <sysexits.h>
#include <memory>

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/weutil/Flags.h"
#include "fboss/platform/weutil/Weutil.h"
#include "fboss/platform/weutil/WeutilDarwin.h"

using namespace facebook::fboss::platform;
using namespace facebook::fboss;
using namespace facebook;

FOLLY_INIT_LOGGING_CONFIG(".=FATAL; default:async=true");

namespace {
void printUsage() {
  std::cout
      << "weutil [--h] [--json] [--eeprom <eeprom-name>] [--path absolute_path_to_eeprom]"
      << std::endl;
  std::cout << "usage examples:" << std::endl;
  std::cout << "    weutil" << std::endl;
  std::cout << "    weutil --eeprom pem" << std::endl;
  std::cout << "    weutil --path /sys/bus/i2c/devices/6-0051/eeprom"
            << std::endl;
  try {
    auto eepromNames = getEepromNames();
    std::cout << "The <eeprom-name>s supported on this platform are: "
              << folly::join(", ", eepromNames) << std::endl;
  } catch (const std::exception& ex) {
    std::cout << "Failed to get supported eeprom names: " << ex.what()
              << std::endl;
  }
}
} // namespace

// If config file is not specified, we detect the platform type and load
// the proper platform config. If no flags/args are specified, weutil will
// output the chassis eeprom. If flags/args are used, check that either
// --eeprom, --path or --h flags are used.
bool validFlags(int argc) {
  if (!FLAGS_path.empty() && !FLAGS_eeprom.empty()) {
    std::cout << "Please use either --path or --eeprom, not both!" << std::endl;
    return false;
  }
  if (argc > 1) {
    std::cout << "Only valid commandline flags are allowed." << std::endl;
    return false;
  }
  return true;
}

// This utility program will output Chassis info for Darwin
int main(int argc, char* argv[]) {
  helpers::init(&argc, &argv);
  std::unique_ptr<WeutilInterface> weutilInstance;

  // Check if the flags/args are valid
  if (!validFlags(argc)) {
    return 1;
  }
  if (FLAGS_h) {
    printUsage();
    return 0;
  }

  try {
    weutilInstance = createWeUtilIntf(FLAGS_eeprom, FLAGS_path);
  } catch (const std::exception& ex) {
    std::cout << "Failed creation of proper parser. " << ex.what() << std::endl;
    return 1;
  }

  if (weutilInstance) {
    try {
      if (FLAGS_json) {
        weutilInstance->printInfoJson();
      } else {
        weutilInstance->printInfo();
      }
    } catch (const std::exception& ex) {
      std::cout << ex.what() << std::endl;
      std::cout << "ERROR: weutil finished with an exception." << std::endl;
      return 1;
    }
  } else {
    XLOG(INFO) << "Exiting with error code";
    return 1;
  }

  return EX_OK;
}
