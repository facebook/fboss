// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/FollyLoggingHandler.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/StructuredLogger.h"
#include "fboss/platform/reboot_cause_service/ConfigValidator.h"
#include "fboss/platform/reboot_cause_service/Flags.h"
#include "fboss/platform/reboot_cause_service/RebootCauseServiceThriftHandler.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::reboot_cause_service;
using facebook::fboss::platform::helpers::StructuredLogger;

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();

  helpers::init(&argc, &argv);

  try {
    auto config = apache::thrift::SimpleJSONSerializer::deserialize<
        reboot_cause_config::RebootCauseConfig>(
        ConfigLib().getRebootCauseServiceConfig());
    if (!ConfigValidator().isValid(config)) {
      throw std::runtime_error("Invalid reboot_cause_service config");
    }
    auto serviceImpl = std::make_shared<RebootCauseServiceImpl>(config);

    if (FLAGS_determine_reboot_cause) {
      serviceImpl->determineRebootCause();
    }

    auto server = std::make_shared<apache::thrift::ThriftServer>();
    auto handler =
        std::make_shared<RebootCauseServiceThriftHandler>(serviceImpl);

    helpers::runThriftService(
        server, handler, "RebootCauseService", FLAGS_thrift_port);
  } catch (const std::exception& ex) {
    StructuredLogger structuredLogger("reboot_cause_service");
    structuredLogger.logAlert("unexpected_crash", ex.what());
    throw;
  }

  return 0;
}
