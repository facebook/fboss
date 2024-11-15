// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <string.h>
#include <sysexits.h>
#include <iostream>
#include <memory>

#include <folly/String.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/weutil/Weutil.h"

using namespace facebook::fboss::platform;
using namespace facebook::fboss;
using namespace facebook;

FOLLY_INIT_LOGGING_CONFIG(".=FATAL; default:async=true");

DEFINE_bool(json, false, "Output in JSON format");
DEFINE_bool(list, false, "List all eeproms in config");
DEFINE_int32(offset, 0, "Offset for eeprom specified by --path");
DEFINE_string(
    eeprom,
    "",
    "EEPROM name or device type, default is chassis eeprom");
DEFINE_string(
    path,
    "",
    "When set, ignore config and read the eeprom specified by the path");

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

int main(int argc, char* argv[]) {
  helpers::initCli(&argc, &argv, "weutil");
  std::unique_ptr<WeutilInterface> weutilInstance;

  if (!validFlags(argc)) {
    return 1;
  }

  if (FLAGS_list) {
    try {
      auto eeproms = getEepromPaths();
      std::cout << folly::join("\n", eeproms) << std::endl;
    } catch (const std::exception& ex) {
      std::cout << "Failed to get list of eeproms: " << ex.what() << std::endl;
    }
    return 0;
  }

  try {
    weutilInstance = createWeUtilIntf(FLAGS_eeprom, FLAGS_path, FLAGS_offset);
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
