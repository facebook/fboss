// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <fb303/FollyLoggingHandler.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <folly/logging/xlog.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/showtech/Utils.h"
#include "fboss/platform/showtech/gen-cpp2/showtech_config_types.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::showtech_config;

int main(int argc, char** argv) {
  helpers::initCli(&argc, &argv, "showtech");

  auto platformName = helpers::PlatformNameLib().getPlatformName();
  std::string showtechConfJson = ConfigLib().getShowtechConfig(platformName);
  auto config =
      apache::thrift::SimpleJSONSerializer::deserialize<ShowtechConfig>(
          showtechConfJson);
  XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      config);

  Utils showtechUtil(config);

  showtechUtil.printHostDetails();
  showtechUtil.printFbossDetails();
  showtechUtil.printWeutilDetails();
  showtechUtil.printFwutilDetails();
  showtechUtil.printLspciDetails();
  showtechUtil.printPortDetails();
  showtechUtil.printSensorDetails();
  showtechUtil.printI2cDetails();
  return 0;
}
