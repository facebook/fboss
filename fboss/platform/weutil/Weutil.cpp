// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <string.h>
#include <sysexits.h>
#include <iostream>
#include <memory>
#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/weutil/WeutilDarwin.h"

using namespace facebook::fboss::platform::helpers;
using namespace facebook::fboss::platform;

std::unique_ptr<WeutilInterface> get_plat_weutil(void) {
  // ToDo: use class PlatformProductInfo to get platform information
  // and pickup the right weutil class accordingly
  return std::make_unique<WeutilDarwin>();
}

/*
 * This utility program will output Chassis info for Darwin
 */
int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  std::unique_ptr<WeutilInterface> weutilInstance = get_plat_weutil();
  if (weutilInstance) {
    weutilInstance->printInfo();
  } else {
    showDeviceInfo();
  }

  return EX_OK;
}
