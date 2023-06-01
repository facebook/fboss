// Copyright 2021- Facebook. All rights reserved.

#include <memory>
#include <mutex>
#include <string>

#include <fb303/FollyLoggingHandler.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/led_service/LedService.h"
#include "fboss/led_service/LedServiceHandler.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook;
using namespace facebook::fboss;
using namespace facebook::fboss::platform;

DEFINE_int32(led_service_port, 5930, "Port for the LED thrift service");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(argc, argv);

  auto serviceImpl = std::make_unique<facebook::fboss::LedService>();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<LedServiceHandler>(std::move(serviceImpl));
  handler->getLedService()->kickStart();

  server->setPort(FLAGS_led_service_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(
      server, handler, "LedService", FLAGS_led_service_port);

  return 0;
}
