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
void printUsage(const WeutilInterface& intf) {
  std::cout
      << "weutil [--h] [--json] [--eeprom <eeprom-name>] [--path absolute_path_to_eeprom]"
      << std::endl;
  std::cout << "usage examples:" << std::endl;
  std::cout << "    weutil" << std::endl;
  std::cout << "    weutil --eeprom pem" << std::endl;
  std::cout << "    weutil --path /sys/bus/i2c/devices/6-0051/eeprom"
            << std::endl;
  std::cout << "The <eeprom-name>s supported on this platform are: "
            << folly::join(" ", intf.getEepromNames()) << std::endl;
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
  try {
    if (!FLAGS_path.empty()) {
      // Path flag is set. We read the eeprom in the absolute path.
      // Neither platform type detection nor config file load is needed.
      weutilInstance = get_meta_eeprom_handler(FLAGS_path);
    } else {
      // Path flag is NOT set. Auto-detect the platform type and
      // load the proper config file
      weutilInstance = get_plat_weutil(FLAGS_eeprom, FLAGS_config_file);
    }
  } catch (const std::exception& ex) {
    std::cout << "Failed creation of proper parser. " << ex.what() << std::endl;
    return 1;
  }

  if (weutilInstance) {
    if (FLAGS_h) {
      printUsage(*weutilInstance);
    } else {
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
    }
  } else {
    XLOG(INFO) << "Exiting with error code";
    return 1;
  }

  return EX_OK;
}
