// Copyright 2004-present Facebook. All Rights Reserved.

#include "common/base/BuildInfo.h"
#include "common/init/Init.h"
#include "common/services/cpp/BuildModule.h"
#include "common/services/cpp/BuildValues.h"
#include "common/services/cpp/Fb303Module.h"

#include <fb303/FollyLoggingHandler.h>
#include <folly/logging/Init.h>

using namespace facebook;
using namespace facebook::services;

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  // Set version info
  gflags::SetVersionString(BuildInfo::toDebugString());

  // Init FB and export build values
  initFacebook(&argc, &argv);
  services::BuildValues::setExportedValues();
  fb303::registerFollyLoggingOptionHandlers();

  return 0;
}
