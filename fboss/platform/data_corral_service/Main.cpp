// Copyright 2004-present Facebook. All Rights Reserved.
//

#include <fb303/FollyLoggingHandler.h>
#include <folly/logging/Init.h>

#include <folly/logging/xlog.h>
#ifndef IS_OSS
#include "common/services/cpp/ServiceFrameworkLight.h"
#endif
#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"
#include "fboss/platform/data_corral_service/SetupDataCorralServiceThrift.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::data_corral_service;

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(argc, argv);

  // ToDo: Setup thrift handler and server
  auto serverHandlerPair = setupThrift();
  __attribute__((unused)) auto server = serverHandlerPair.first;
  __attribute__((unused)) auto handler = serverHandlerPair.second;

#ifndef IS_OSS
  facebook::services::ServiceFrameworkLight service("Data Corral Service");
  // Finally, run the Thrift server
  runServer(service, server, handler.get());
#endif
  XLOG(INFO) << "data_corral_service exits";

  return 0;
}
