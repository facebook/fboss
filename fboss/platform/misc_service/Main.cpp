#include "common/base/BuildInfo.h"
#include "common/init/Init.h"

#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#include <folly/logging/xlog.h>
#include "common/services/cpp/BuildValues.h"
#include "common/services/cpp/ServiceFrameworkLight.h"
#include "fboss/platform/misc_service/MiscServiceThriftHandler.h"
#include "fboss/platform/misc_service/SetupMiscServiceThrift.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::misc_service;

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
  auto [server, handler] = setupThrift();

  folly::FunctionScheduler scheduler;

  scheduler.start();

  facebook::services::ServiceFrameworkLight service("Misc Service");
  // Finally, run the Thrift server
  runServer(service, server, handler.get());
  XLOG(INFO) << "misc_service exits";

  return 0;
}
