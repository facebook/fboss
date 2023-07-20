// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <string.h>
#include <sysexits.h>
#include <memory>

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/weutil/Flags.h"
#include "fboss/platform/weutil/Weutil.h"
#include "fboss/platform/weutil/WeutilDarwin.h"

using namespace facebook::fboss::platform::helpers;
using namespace facebook::fboss::platform;
using namespace facebook::fboss;
using namespace facebook;

// This utility program will output Chassis info for Darwin
int main(int argc, char* argv[]) {
  std::unique_ptr<WeutilInterface> weutilInstance;
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  weutilInstance = get_plat_weutil(FLAGS_eeprom);

  if (weutilInstance) {
    if (FLAGS_h) {
      weutilInstance->printUsage();
    } else if (FLAGS_json) {
      weutilInstance->printInfoJson();
    } else {
      weutilInstance->printInfo();
    }
  } else {
    showDeviceInfo();
  }

  return EX_OK;
}
