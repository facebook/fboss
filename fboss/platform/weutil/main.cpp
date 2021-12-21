// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <string.h>
#include <sysexits.h>
#include <memory>
#include "common/fbwhoami/FbWhoAmI.h"
#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/weutil/WeutilDarwin.h"

using namespace facebook::fboss::platform::helpers;
using namespace facebook::fboss::platform;

DEFINE_bool(json, false, "output in JSON format");

std::unique_ptr<WeutilInterface> get_plat_weutil(void) {
  // Get the model name from FbWhoAmI instead of from class PlatformProductInfo
  // to omit catching initialization issues
  auto modelName = facebook::FbWhoAmI::getModelName();
  if (modelName.find("Darwin") == 0 || modelName.find("DARWIN") == 0) {
    return std::make_unique<WeutilDarwin>();
  }

  XLOG(INFO) << "The platform (" << modelName << ") is not supported"
             << std::endl;
  return nullptr;
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
    if (FLAGS_json) {
      weutilInstance->printInfoJson();
    } else {
      weutilInstance->printInfo();
    }
  } else {
    showDeviceInfo();
  }

  return EX_OK;
}
