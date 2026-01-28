// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <algorithm>
#include <ranges>
#include <set>
#include <vector>

#include <CLI/CLI.hpp>
#include <fmt/core.h>
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
enum class Disruptiveness {
  NONDISRUPTIVE,
  DISRUPTIVE,
};
using FunctionWithDisruptiveFlag =
    std::pair<std::function<void(Utils&)>, Disruptiveness>;
const std::vector<std::pair<std::string, FunctionWithDisruptiveFlag>>
    DETAIL_FUNCTIONS = {
        {"host",
         {[](Utils& util) { util.printHostDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"fboss",
         {[](Utils& util) { util.printFbossDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"powergood",
         {[](Utils& util) { util.printPowerGoodDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"weutil",
         {[](Utils& util) { util.printWeutilDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"fwutil",
         {[](Utils& util) { util.printFwutilDetails(); },
          Disruptiveness::DISRUPTIVE}},
        {"lspci",
         {[](Utils& util) { util.printLspciDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"port",
         {[](Utils& util) { util.printPortDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"sensor",
         {[](Utils& util) { util.printSensorDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"psu",
         {[](Utils& util) { util.printPsuDetails(); },
          Disruptiveness::DISRUPTIVE}},
        {"pem",
         {[](Utils& util) { util.printPemDetails(); },
          Disruptiveness::DISRUPTIVE}},
        {"fan",
         {[](Utils& util) { util.printFanDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"fanspinner",
         {[](Utils& util) { util.printFanspinnerDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"gpio",
         {[](Utils& util) { util.printGpioDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"i2c",
         {[](Utils& util) { util.printI2cDetails(); },
          Disruptiveness::DISRUPTIVE}},
        {"i2cdump",
         {[](Utils& util) { util.printI2cDumpDetails(); },
          Disruptiveness::DISRUPTIVE}},
        {"nvme",
         {[](Utils& util) { util.printNvmeDetails(); },
          Disruptiveness::NONDISRUPTIVE}},
        {"logs",
         {[](Utils& util) { util.printLogs(); },
          Disruptiveness::NONDISRUPTIVE}},
};

std::set<std::string> getValidDetailNames() {
  std::set<std::string> result{"all"};
  for (const auto& [name, func] : DETAIL_FUNCTIONS) {
    if (func.second == Disruptiveness::DISRUPTIVE) {
      result.insert(name + "(disruptive)");
    } else {
      result.insert(name);
    }
  }
  return result;
}

void executeSingleDetail(
    Utils& showtechUtil,
    const std::string& name,
    const FunctionWithDisruptiveFlag& detailDescriptor,
    bool disruptiveMode) {
  if (disruptiveMode ||
      detailDescriptor.second == Disruptiveness::NONDISRUPTIVE) {
    detailDescriptor.first(showtechUtil);
  } else {
    std::string upperCaseName = name;
    std::transform(
        upperCaseName.begin(),
        upperCaseName.end(),
        upperCaseName.begin(),
        ::toupper);
    std::cout
        << fmt::format(
               "##### Skipping: {}(Dirsuptive), in non-disruptive mode #####",
               upperCaseName)
        << std::endl;
  }
}

void executeRequestedDetails(
    Utils& showtechUtil,
    const std::vector<std::string>& requestedDetails,
    bool disruptiveMode) {
  bool runAll =
      std::ranges::find(requestedDetails, "all") != requestedDetails.end();

  if (runAll) {
    XLOG(INFO) << "Running all detail functions";
    for (const auto& [name, funcWithFlag] : DETAIL_FUNCTIONS) {
      executeSingleDetail(showtechUtil, name, funcWithFlag, disruptiveMode);
    }
  } else {
    for (const auto& requestedDetail : requestedDetails) {
      auto it = std::ranges::find_if(DETAIL_FUNCTIONS, [&](const auto& pair) {
        return pair.first == requestedDetail;
      });
      if (it != DETAIL_FUNCTIONS.end()) {
        executeSingleDetail(
            showtechUtil, it->first, it->second, disruptiveMode);
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
  bool disruptiveMode = false;

  app.add_flag(
      "--disruptive",
      disruptiveMode,
      "Enable Disruptive Mode to run disruptive functions");
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
    executeRequestedDetails(showtechUtil, detailsArg, disruptiveMode);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Error during showtech execution: " << e.what();
    return 1;
  }

  return 0;
}
