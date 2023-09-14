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

// This utility program will output Chassis info for Darwin
int main(int argc, char* argv[]) {
  helpers::init(argc, argv);
  std::unique_ptr<WeutilInterface> weutilInstance;

  weutilInstance = get_plat_weutil(FLAGS_eeprom, FLAGS_config_file);

  if (weutilInstance) {
    if (FLAGS_h) {
      weutilInstance->printUsage();
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
