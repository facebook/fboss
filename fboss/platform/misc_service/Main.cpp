#include "common/base/BuildInfo.h"
#include "common/init/Init.h"

#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#include <folly/logging/xlog.h>
#include "common/services/cpp/BuildValues.h"

using namespace facebook;
using namespace facebook::services;
// using namespace facebook::fboss::platform;

int main(int argc, char** argv) {
  // Set version info
  gflags::SetVersionString(BuildInfo::toDebugString());
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  // Init FB and export build values
  initFacebook(&argc, &argv);
  services::BuildValues::setExportedValues();
  fb303::registerFollyLoggingOptionHandlers();

  // ToDo: Setup thrift handler and server

  folly::FunctionScheduler scheduler;

  scheduler.start();

  // ToDo: run the Thrift server
  XLOG(INFO) << "misc_service exits";

  return 0;
}
