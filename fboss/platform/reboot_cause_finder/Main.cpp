// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/FollyLoggingHandler.h>
#include <folly/logging/xlog.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <optional>
#include <string>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/reboot_cause_finder/ConfigValidator.h"
#include "fboss/platform/reboot_cause_finder/RebootCauseFinderImpl.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::reboot_cause_finder;

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();

  helpers::init(&argc, &argv);

  std::optional<std::string> configJson;
  try {
    configJson = ConfigLib().getRebootCauseFinderConfig();
  } catch (const std::exception& ex) {
    if (!FLAGS_config_file.empty()) {
      XLOG(ERR) << "Failed to read --config_file '" << FLAGS_config_file
                << "': " << ex.what();
      return 1;
    }
    XLOG(INFO) << "No reboot_cause_finder config for this platform; "
                  "continuing with no hardware providers. "
               << ex.what();
  }

  reboot_cause_config::RebootCauseConfig config;
  if (configJson.has_value()) {
    try {
      config = apache::thrift::SimpleJSONSerializer::deserialize<
          reboot_cause_config::RebootCauseConfig>(*configJson);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Failed to parse reboot_cause_finder config: " << ex.what();
      return 1;
    }
    if (!ConfigValidator().isValid(config)) {
      XLOG(ERR) << "Invalid reboot_cause_finder config";
      return 1;
    }
  }

  RebootCauseFinderImpl(config).determineRebootCause();

  return 0;
}
