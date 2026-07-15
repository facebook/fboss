// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/FollyLoggingHandler.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

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

  auto config = apache::thrift::SimpleJSONSerializer::deserialize<
      reboot_cause_config::RebootCauseConfig>(
      ConfigLib().getRebootCauseFinderConfig());
  if (!ConfigValidator().isValid(config)) {
    throw std::runtime_error("Invalid reboot_cause_finder config");
  }
  RebootCauseFinderImpl(config).determineRebootCause();

  return 0;
}
