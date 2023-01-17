// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/fbdevd/darwin/DarwinDeviceTopology.h"
#include "fboss/platform/fbdevd/if/gen-cpp2/FbdevManager.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::fbdevd;

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  // Init FB and export build values
  helpers::init(argc, argv);
  fb303::registerFollyLoggingOptionHandlers();

  // Set up scheduler.
  folly::FunctionScheduler scheduler;
  scheduler.start();

  return 0;
}
