// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <fb303/FollyLoggingHandler.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/platform_manager/PlatformExplorer.h"
#include "fboss/platform/platform_manager/PlatformManagerHandler.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

DEFINE_int32(thrift_port, 5975, "Port for the thrift service");

DEFINE_int32(
    explore_interval_s,
    60,
    "Frequency at which the platform needs to be explored");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(argc, argv);

  PlatformExplorer platformExplorer(
      (std::chrono::seconds(FLAGS_explore_interval_s)));

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<PlatformManagerHandler>();
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(
      server, handler, "PlatformManagerService", FLAGS_thrift_port);

  return 0;
}
