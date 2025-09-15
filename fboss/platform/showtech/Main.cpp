// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include <CLI/CLI.hpp>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/showtech/Utils.h"
#include "fboss/platform/showtech/gen-cpp2/showtech_config_types.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::showtech_config;

namespace {

const std::unordered_map<std::string, std::function<void(Utils&)>>
    DETAIL_FUNCTIONS = {
        {"host", [](Utils& util) { util.printHostDetails(); }},
        {"fboss", [](Utils& util) { util.printFbossDetails(); }},
        {"weutil", [](Utils& util) { util.printWeutilDetails(); }},
        {"fwutil", [](Utils& util) { util.printFwutilDetails(); }},
        {"lspci", [](Utils& util) { util.printLspciDetails(); }},
        {"port", [](Utils& util) { util.printPortDetails(); }},
        {"sensor", [](Utils& util) { util.printSensorDetails(); }},
        {"psu", [](Utils& util) { util.printPsuDetails(); }},
        {"i2c", [](Utils& util) { util.printI2cDetails(); }},
};

std::unordered_set<std::string> getValidDetailNames() {
  auto keys = std::views::keys(DETAIL_FUNCTIONS) |
      std::views::transform([](const auto& key) { return key; });
  std::unordered_set<std::string> result{"all"};
  result.insert(keys.begin(), keys.end());
  return result;
}

void executeRequestedDetails(
    Utils& showtechUtil,
    const std::vector<std::string>& requestedDetails) {
  bool runAll =
      std::ranges::find(requestedDetails, "all") != requestedDetails.end();

  if (runAll) {
    XLOG(INFO) << "Running all detail functions";
    for (const auto& [name, func] : DETAIL_FUNCTIONS) {
      func(showtechUtil);
    }
  } else {
    for (const auto& requestedDetail : requestedDetails) {
      if (auto it = DETAIL_FUNCTIONS.find(requestedDetail);
          it != DETAIL_FUNCTIONS.end()) {
        XLOG(INFO) << "Executing: " << requestedDetail;
        it->second(showtechUtil);
      }
    }
  }
}

} // namespace

int main(int argc, char** argv) {
  CLI::App app{"Showtech utility for collecting system diagnostics"};
  app.set_version_flag("--version", helpers::getBuildVersion());

  std::vector<std::string> detailsArg = {"all"};
  app.add_option(
         "--details", detailsArg, "Comma-separated list of details to print")
      ->default_val(std::vector<std::string>{"all"})
      ->delimiter(',')
      ->check(CLI::IsMember(getValidDetailNames()));

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  try {
    auto platformNameOpt = helpers::PlatformNameLib().getPlatformName();
    if (!platformNameOpt.has_value()) {
      XLOG(ERR) << "Failed to get platform name";
      return 1;
    }

    const auto& platformName = platformNameOpt.value();
    XLOG(INFO) << "Running showtech for platform: " << platformName;

    std::string showtechConfJson = ConfigLib().getShowtechConfig(platformName);
    auto config =
        apache::thrift::SimpleJSONSerializer::deserialize<ShowtechConfig>(
            showtechConfJson);

    Utils showtechUtil(config);
    executeRequestedDetails(showtechUtil, detailsArg);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Error during showtech execution: " << e.what();
    return 1;
  }

  return 0;
}
