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

#include "fboss/platform/fixmyfboss/ResultPrinter.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/platform_checks/PlatformCheck.h"
#include "fboss/platform/platform_checks/checks/MacAddressCheck.h"
#include "fboss/platform/platform_checks/checks/PciDeviceCheck.h"
#include "fboss/platform/platform_checks/checks/PowerResetCheck.h"

using namespace facebook::fboss::platform;

namespace {

// Create all platform checks
std::vector<std::unique_ptr<platform_checks::PlatformCheck>> createAllChecks() {
  std::vector<std::unique_ptr<platform_checks::PlatformCheck>> checks;
  checks.push_back(std::make_unique<platform_checks::MacAddressCheck>());
  checks.push_back(std::make_unique<platform_checks::PciDeviceCheck>());
  checks.push_back(
      std::make_unique<platform_checks::RecentManualRebootCheck>());
  checks.push_back(std::make_unique<platform_checks::RecentKernelPanicCheck>());
  checks.push_back(
      std::make_unique<platform_checks::WatchdogDidNotStopCheck>());
  return checks;
}

// Filter checks by platform
std::vector<platform_checks::PlatformCheck*> getChecksForPlatform(
    const std::vector<std::unique_ptr<platform_checks::PlatformCheck>>& checks,
    const std::string& platformName) {
  std::vector<platform_checks::PlatformCheck*> filteredChecks;
  for (const auto& check : checks) {
    auto supportedPlatforms = check->getSupportedPlatforms();
    // Empty set means all platforms are supported
    if (supportedPlatforms.empty() ||
        supportedPlatforms.count(platformName) > 0) {
      filteredChecks.push_back(check.get());
    }
  }
  return filteredChecks;
}

std::vector<platform_checks::CheckResult> runAllChecks(
    const std::vector<platform_checks::PlatformCheck*>& checks) {
  std::vector<platform_checks::CheckResult> results;

  for (auto* check : checks) {
    XLOG(DBG2) << "Running check: " << check->getDescription();
    try {
      auto result = check->run();
      XLOG(DBG2) << "Check " << check->getDescription()
                 << " completed with status: "
                 << static_cast<int>(*result.status());
      results.push_back(std::move(result));
    } catch (const std::exception& e) {
      // Convert exceptions to ERROR results
      XLOG(ERR) << "Exception in check " << check->getDescription() << ": "
                << e.what();
      platform_checks::CheckResult errorResult;
      errorResult.checkType() = check->getType();
      errorResult.checkName() = check->getName();
      errorResult.status() = platform_checks::CheckStatus::ERROR;
      errorResult.errorMessage() =
          std::string("Exception during check execution: ") + e.what();
      results.push_back(std::move(errorResult));
    }
  }
  return results;
}

void listChecks(const std::string& platformName) {
  auto allChecks = createAllChecks();
  auto checksForPlatform = getChecksForPlatform(allChecks, platformName);

  std::cout << "List of checks:\n";
  int index = 1;

  for (const auto& check : allChecks) {
    bool willRun =
        std::find_if(
            checksForPlatform.begin(), checksForPlatform.end(), [&](auto* c) {
              return c == check.get();
            }) != checksForPlatform.end();
    std::string runIndicator = willRun ? "" : " (skipped)";
    std::cout << index++ << ". " << check->getName() << ": "
              << check->getDescription() << runIndicator << "\n";
  }
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
    const std::string& platformName) {
  XLOG(INFO) << "Starting fixmyfboss checks for platform: " << platformName;

  auto allChecks = createAllChecks();
  auto checks = getChecksForPlatform(allChecks, platformName);

  XLOG(INFO) << "Found " << checks.size()
             << " applicable checks for this platform";

  fixmyfboss::ResultPrinter printer;
  printer.printProgress(
      "Platform: " + platformName + " - Running " +
      std::to_string(checks.size()) + " checks");

  auto results = runAllChecks(checks);
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

  auto platformNameOpt = helpers::PlatformNameLib().getPlatformName();
  if (!platformNameOpt.has_value()) {
    XLOG(ERR) << "Failed to determine platform name";
    return EXIT_FAILURE;
  }
  const std::string& platformName = platformNameOpt.value();

  // Handle --list-checks mode
  if (listChecksFlag) {
    listChecks(platformName);
    return EXIT_SUCCESS;
  }

  // Run checks and print results
  auto results = runChecks(platformName);
  fixmyfboss::ResultPrinter printer;
  printer.printSummary(results);
  printer.printDetails(results);

  bool allPassed =
      std::all_of(results.begin(), results.end(), [](const auto& result) {
        return *result.status() == platform_checks::CheckStatus::OK;
      });

  return allPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
