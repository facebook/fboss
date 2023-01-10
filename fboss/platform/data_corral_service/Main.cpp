// Copyright 2004-present Facebook. All Rights Reserved.
//

#include <fb303/FollyLoggingHandler.h>
#include <folly/logging/Init.h>

#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"
#include "fboss/platform/data_corral_service/Flags.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::data_corral_service;

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(argc, argv);

  auto serviceImpl = std::make_shared<DataCorralServiceImpl>();
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<DataCorralServiceThriftHandler>(serviceImpl);

  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(
      server, handler, "DataCorralService", FLAGS_thrift_port);

  return 0;
}
