// Copyright 2004-present Facebook. All Rights Reserved.

#include <fb303/FollyLoggingHandler.h>
#include <folly/logging/Init.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/rackmon/RackmonThriftHandler.h"

using namespace facebook;
using namespace facebook::fboss::platform;

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

DEFINE_int32(port, 5973, "Port for the thrift service");

int main(int argc, char** argv) {
  helpers::init(argc, argv);
  fb303::registerFollyLoggingOptionHandlers();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<rackmonsvc::ThriftHandler>();

  server->setPort(FLAGS_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(server, handler, "DataCorralService", FLAGS_port);

  return 0;
}
