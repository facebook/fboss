// Copyright 2004-present Facebook. All Rights Reserved.
//

#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#include <folly/logging/xlog.h>
#ifndef IS_OSS
#include "common/services/cpp/ServiceFrameworkLight.h"
#endif
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/misc_service/MiscServiceThriftHandler.h"
#include "fboss/platform/misc_service/SetupMiscServiceThrift.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::misc_service;

int main(int argc, char** argv) {
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  fb303::registerFollyLoggingOptionHandlers();

  helpers::fbInit(argc, argv);

  // ToDo: Setup thrift handler and server
  auto [server, handler] = setupThrift();

  folly::FunctionScheduler scheduler;

  scheduler.start();

#ifndef IS_OSS
  facebook::services::ServiceFrameworkLight service("Misc Service");
  // Finally, run the Thrift server
  runServer(service, server, handler.get());
#endif
  XLOG(INFO) << "misc_service exits";

  return 0;
}
