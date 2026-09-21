// Copyright 2004-present Facebook. All Rights Reserved.
//

#include <fb303/FollyLoggingHandler.h>
#include <folly/executors/FunctionScheduler.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/data_corral_service/ConfigValidator.h"
#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"
#include "fboss/platform/data_corral_service/FruPresenceExplorer.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/helpers/StructuredLogger.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::data_corral_service;
using facebook::fboss::platform::helpers::StructuredLogger;

DEFINE_int32(
    refresh_interval_ms,
    10000,
    "How often to refresh FRU and program LED.");

DEFINE_int32(thrift_port, 5971, "Port for the thrift service");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(&argc, &argv);

  auto platformName = helpers::PlatformNameLib().getPlatformName();

  try {
    LedManagerConfig config;
    auto configJson = ConfigLib().getLedManagerConfig(platformName);
    apache::thrift::SimpleJSONSerializer::deserialize<LedManagerConfig>(
        configJson, config);

    if (!ConfigValidator().isValid(config)) {
      XLOG(ERR) << "Invalid LedManager config";
      throw std::runtime_error("Invalid LedManager config");
    }

    auto ledManager = std::make_shared<LedManager>(
        *config.systemLedConfig(), *config.fruTypeLedConfigs());
    auto fruPresenceExplorer =
        std::make_shared<FruPresenceExplorer>(*config.fruConfigs(), ledManager);

    folly::FunctionScheduler scheduler;
    scheduler.addFunction(
        [&]() { fruPresenceExplorer->detectFruPresence(); },
        std::chrono::milliseconds(FLAGS_refresh_interval_ms),
        "PresenceDetection");
    scheduler.start();

    auto server = std::make_shared<apache::thrift::ThriftServer>();
    auto handler = std::make_shared<DataCorralServiceThriftHandler>();

    helpers::runThriftService(
        server, handler, "DataCorralService", FLAGS_thrift_port);
  } catch (const std::exception& ex) {
    StructuredLogger logger("data_corral_service");
    logger.logAlert("unexpected_crash", ex.what());
    throw;
  }

  XLOG(INFO) << "================ STOPPED PLATFORM BINARY ================";
  return 0;
}
