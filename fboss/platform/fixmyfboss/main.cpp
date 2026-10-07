/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <cstdlib>
#include <iostream>
#include <memory>

#include <CLI/CLI.hpp>
#include <folly/logging/LoggerDB.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/fixmyfboss/CheckRegistry.h"
#include "fboss/platform/fixmyfboss/CheckRunner.h"
#include "fboss/platform/fixmyfboss/ResultPrinter.h"

using namespace facebook::fboss::platform;

namespace {

void listChecks(
    const std::vector<std::unique_ptr<platform_checks::PlatformCheck>>& checks,
    const fixmyfboss::CheckRunner& runner) {
  std::cout << "List of checks:\n";
  int index = 1;
  for (const auto& check : checks) {
    std::string runIndicator =
        runner.appliesToPlatform(*check) ? "" : " (skipped)";
    std::cout << index++ << ". " << check->getName() << ": "
              << check->getDescription() << runIndicator << "\n";
  }
}

platform_checks::RemoteHost::Transport parseTransport(
    const std::string& transport) {
  if (transport == "ssh") {
    return platform_checks::RemoteHost::Transport::SSH;
  }
  if (transport == "sush2") {
    return platform_checks::RemoteHost::Transport::SUSH2;
  }
  return platform_checks::RemoteHost::detectTransport();
}

void configureLogging(bool debugFlag, bool verboseFlag) {
  // Set default log level to WARNING
  folly::LoggerDB::get().setLevel("", folly::LogLevel::WARN);

  // Update log level based on flags
  if (debugFlag) {
    folly::LoggerDB::get().setLevel("", folly::LogLevel::DBG);
  } else if (verboseFlag) {
    folly::LoggerDB::get().setLevel("", folly::LogLevel::INFO);
  }
}

std::vector<platform_checks::CheckResult> runChecks(
    const std::vector<std::unique_ptr<platform_checks::PlatformCheck>>& checks,
    const fixmyfboss::CheckRunner& runner,
    const std::string& platformName) {
  XLOG(INFO) << "Starting fixmyfboss checks for platform: " << platformName;

  fixmyfboss::ResultPrinter printer;
  printer.printProgress(
      "Platform: " + platformName + " - Running " +
      std::to_string(checks.size()) + " checks");

  auto results = runner.run(checks);
  printer.clearLine();
  return results;
}

} // namespace

int main(int argc, char* argv[]) {
  CLI::App app{
      "fixmyfboss - Diagnose and fix FBOSS device issues", "fixmyfboss"};

  // Add command line options
  bool listChecksFlag = false;
  app.add_flag(
         "--list-checks",
         listChecksFlag,
         "Show list of available checks and exit")
      ->group("Information");

  std::string hostname;
  app.add_option(
         "--hostname",
         hostname,
         "Diagnose this switch over SSH instead of the local machine")
      ->group("Remote");

  std::string transport = "auto";
  app.add_option(
         "--transport",
         transport,
         "How to reach --hostname: ssh, sush2, or auto (sush2 if installed)")
      ->check(CLI::IsMember({"auto", "ssh", "sush2"}))
      ->group("Remote");

  bool verboseFlag = false;
  app.add_flag(
         "-v,--verbose", verboseFlag, "Enable verbose logging (INFO level)")
      ->group("Logging");

  bool debugFlag = false;
  app.add_flag("-d,--debug", debugFlag, "Enable debug logging (DBG level)")
      ->group("Logging");

  // Parse command line
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  configureLogging(debugFlag, verboseFlag);

  fixmyfboss::ConnectOptions connectOptions;
  if (!hostname.empty()) {
    connectOptions.hostname = hostname;
    connectOptions.transport = parseTransport(transport);
  }
  fixmyfboss::CheckEnvironment env;
  try {
    env = fixmyfboss::createEnvironment(connectOptions);
  } catch (const std::exception& ex) {
    XLOG(ERR) << ex.what();
    return EXIT_FAILURE;
  }
  auto checks = fixmyfboss::createAllChecks(env);
  const fixmyfboss::CheckRunner runner(env.platformName);

  // Handle --list-checks mode
  if (listChecksFlag) {
    listChecks(checks, runner);
    return EXIT_SUCCESS;
  }

  // Run checks and print results
  auto results = runChecks(checks, runner, env.platformName);
  fixmyfboss::ResultPrinter printer(std::cout, verboseFlag || debugFlag);
  printer.printSummary(results);
  printer.printDetails(results);

  bool allPassed =
      std::all_of(results.begin(), results.end(), [](const auto& result) {
        return *result.status() == platform_checks::CheckStatus::OK ||
            *result.status() == platform_checks::CheckStatus::SKIPPED;
      });

  return allPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
