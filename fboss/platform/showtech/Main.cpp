// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <algorithm>
#include <ranges>
#include <set>
#include <vector>

#include <CLI/CLI.hpp>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"

#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/showtech/Utils.h"
#include "fboss/platform/showtech/gen-cpp2/showtech_config_types.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::showtech_config;

namespace {

const std::vector<std::pair<std::string, std::function<void(Utils&)>>>
    DETAIL_FUNCTIONS = {
        {"host", [](Utils& util) { util.printHostDetails(); }},
        {"fboss", [](Utils& util) { util.printFbossDetails(); }},
        {"powergood", [](Utils& util) { util.printPowerGoodDetails(); }},
        {"weutil", [](Utils& util) { util.printWeutilDetails(); }},
        {"fwutil", [](Utils& util) { util.printFwutilDetails(); }},
        {"lspci", [](Utils& util) { util.printLspciDetails(); }},
        {"port", [](Utils& util) { util.printPortDetails(); }},
        {"sensor", [](Utils& util) { util.printSensorDetails(); }},
        {"psu", [](Utils& util) { util.printPsuDetails(); }},
        {"pem", [](Utils& util) { util.printPemDetails(); }},
        {"fan", [](Utils& util) { util.printFanDetails(); }},
        {"fanspinner", [](Utils& util) { util.printFanspinnerDetails(); }},
        {"gpio", [](Utils& util) { util.printGpioDetails(); }},
        {"i2c", [](Utils& util) { util.printI2cDetails(); }},
        {"i2cdump", [](Utils& util) { util.printI2cDumpDetails(); }},
        {"nvme", [](Utils& util) { util.printNvmeDetails(); }},
        {"logs", [](Utils& util) { util.printLogs(); }},
};

std::set<std::string> getValidDetailNames() {
  std::set<std::string> result{"all"};
  for (const auto& [name, func] : DETAIL_FUNCTIONS) {
    result.insert(name);
  }
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
      auto it = std::ranges::find_if(DETAIL_FUNCTIONS, [&](const auto& pair) {
        return pair.first == requestedDetail;
      });
      if (it != DETAIL_FUNCTIONS.end()) {
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

  std::vector<std::string> detailsArg = {};
  std::string configFilePath;

  app.add_option("--details", detailsArg, folly::stripLeftMargin(R"(
           Comma-separated list of details to print.
           Use specific section details to avoid printing too much data.
           Use 'all' only if necessary. 'all' can take a long time to run.
           Recommended usage with `all` is to redirect the output to a file.
         )"))
      ->delimiter(',')
      ->required()
      ->check(CLI::IsMember(getValidDetailNames()));

  app.add_option(
      "--config_file", configFilePath, "Path to the showtech config file");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  try {
    std::string showtechConfJson =
        ConfigLib(configFilePath).getShowtechConfig();
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
