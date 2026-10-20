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
#include <optional>
#include <stdexcept>

#include <CLI/CLI.hpp>
#include <folly/String.h>
#include <folly/logging/LoggerDB.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/fixmyfboss/CheckRegistry.h"
#include "fboss/platform/fixmyfboss/CheckRunner.h"
#include "fboss/platform/fixmyfboss/PasswordPrompt.h"
#include "fboss/platform/fixmyfboss/ResultPrinter.h"

using namespace facebook::fboss::platform;

namespace {

void listChecks(const std::vector<fixmyfboss::PlannedCheck>& plan) {
  std::cout << "List of checks:\n";
  int index = 1;
  for (const auto& planned : plan) {
    std::string runIndicator;
    if (planned.skipReason) {
      runIndicator = " (skipped: " + *planned.skipReason + ")";
    } else if (planned.skipReasonError) {
      runIndicator = " (error: " + *planned.skipReasonError + ")";
    }
    const auto& check = planned.check.get();
    std::cout << index++ << ". " << check.getName() << ": "
              << check.getDescription() << runIndicator << "\n";
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

void printJson(
    std::vector<platform_checks::CheckResult> results,
    const fixmyfboss::CheckEnvironment& env) {
  // Cannot happen: createEnvironment() always sets x86.
  if (!env.x86) {
    throw std::logic_error("CheckEnvironment has no x86 host");
  }
  platform_checks::CheckReport report;
  report.results() = std::move(results);
  report.platformName() = env.platformName;
  report.target() = env.x86->name();
  std::cout << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                   report)
            << "\n";
}

// Makes it obvious, before anything happens, that the switch will be power
// cycled or rebooted.
void warnAboutDisruptiveChecks(
    const std::vector<fixmyfboss::PlannedCheck>& plan,
    const fixmyfboss::CheckEnvironment& env) {
  std::vector<std::string> names;
  for (const auto& planned : plan) {
    if (planned.check.get().isDisruptive() && planned.appliesToPlatform &&
        !planned.skipReason && !planned.skipReasonError) {
      names.push_back(planned.check.get().getName());
    }
  }
  if (!names.empty()) {
    XLOG(WARN) << "Running disruptive checks on " << env.x86->name() << ": "
               << folly::join(", ", names);
  }
}

std::vector<platform_checks::CheckResult> runChecks(
    const std::vector<fixmyfboss::PlannedCheck>& plan,
    const fixmyfboss::CheckRunner& runner,
    const std::string& platformName,
    bool showProgress) {
  XLOG(INFO) << "Starting fixmyfboss checks for platform: " << platformName;
  if (!showProgress) {
    return runner.run(plan);
  }
  fixmyfboss::ResultPrinter printer;
  auto results = runner.run(plan, [&](int index, int total, const auto& check) {
    printer.printProgress(
        "Platform: " + platformName + " - Running " + std::to_string(index) +
        "/" + std::to_string(total) + ": " + check.getName());
  });
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

  std::string bmcHostname;
  app.add_option(
         "--bmc-hostname",
         bmcHostname,
         "BMC to check over SSH (default with --hostname: <hostname>-oob)")
      ->group("Remote");

  bool noBmc = false;
  app.add_flag("--no-bmc", noBmc, "Skip all checks that need the BMC")
      ->group("Remote")
      ->excludes("--bmc-hostname");

  std::string checkFilter;
  app.add_option(
         "--checks",
         checkFilter,
         "Only run checks whose name matches this regex (case-insensitive)")
      ->group("Checks");

  bool includeDisruptive = false;
  app.add_flag(
         "--include-disruptive",
         includeDisruptive,
         "Also run checks that reboot or power cycle the switch")
      ->group("Checks");

  std::string hwTestsDir;
  app.add_option(
         "--hw-tests-dir",
         hwTestsDir,
         "Directory with prebuilt platform hw tests and OpenBMC tests2/, "
         "copied to the switch to run (disruptive)")
      ->check(CLI::ExistingDirectory)
      ->group("Checks");

  std::string bmcPlatform;
  app.add_option(
         "--bmc-platform",
         bmcPlatform,
         "Platform for the BMC CIT runner (default: lowercase platform name)")
      ->group("Checks");

  bool jsonFlag = false;
  app.add_flag(
         "--json",
         jsonFlag,
         "Print results as a JSON CheckReport instead of a summary")
      ->group("Output");

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

  fixmyfboss::ConnectOptions connectOptions{
      .noBmc = noBmc, .transport = parseTransport(transport)};
  if (!hostname.empty()) {
    connectOptions.hostname = hostname;
  }
  if (!bmcHostname.empty()) {
    connectOptions.bmcHostname = bmcHostname;
  }
  // Never taken from the command line, which is logged.
  connectOptions.askPassword = fixmyfboss::readPasswordFromTerminal;
  fixmyfboss::CheckEnvironment env;
  try {
    env = fixmyfboss::createEnvironment(connectOptions);
  } catch (const std::exception& ex) {
    XLOG(ERR) << ex.what();
    return EXIT_FAILURE;
  }
  if (!hwTestsDir.empty()) {
    env.hwTestsDir = hwTestsDir;
  }
  env.bmcPlatform = bmcPlatform;
  if (env.bmcPlatform.empty()) {
    env.bmcPlatform = env.platformName;
    folly::toLowerAscii(env.bmcPlatform);
  }
  auto checks = fixmyfboss::createAllChecks(env);
  fixmyfboss::RunOptions runOptions{.includeDisruptive = includeDisruptive};
  if (!checkFilter.empty()) {
    runOptions.nameFilter = checkFilter;
  }
  std::optional<fixmyfboss::CheckRunner> runner;
  try {
    runner.emplace(env.platformName, runOptions);
  } catch (const std::invalid_argument& ex) {
    XLOG(ERR) << "Invalid --checks regex '" << checkFilter
              << "': " << ex.what();
    return EXIT_FAILURE;
  }

  // Handle --list-checks mode
  if (listChecksFlag) {
    listChecks(runner->plan(checks));
    return EXIT_SUCCESS;
  }

  // Run checks and print results
  const auto plan = runner->plan(checks);
  warnAboutDisruptiveChecks(plan, env);
  auto results = runChecks(plan, *runner, env.platformName, !jsonFlag);
  if (jsonFlag) {
    printJson(results, env);
  } else {
    fixmyfboss::ResultPrinter printer(std::cout, verboseFlag || debugFlag);
    printer.printSummary(results);
    printer.printDetails(results);
  }

  bool allPassed =
      std::all_of(results.begin(), results.end(), [](const auto& result) {
        return *result.status() == platform_checks::CheckStatus::OK ||
            *result.status() == platform_checks::CheckStatus::SKIPPED;
      });

  return allPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
