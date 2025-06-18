//  Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

#include <CLI/CLI.hpp>
#include <filesystem>
#include "common/base/BuildInfo.h"
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/fw_util/fw_util_helpers.h"

using namespace facebook::fboss::platform::fw_util;
using namespace facebook::fboss::platform;

/*
 * This utility will perform firmware upgrade for
 * all BMC Lite platform. Firmware upgrade will
 * include cpld, fpga, and bios.
 */
int main(int argc, char* argv[]) {
  // TODO: Add file lock to prevent multiple instance of fw_util from running
  // simultaneously.
  folly::initLoggingOrDie();
  CLI::App app{"Firmware utility for FBOSS platforms"};
  std::string fw_target_name;
  std::string fw_action;
  std::string fw_binary_file;
  bool verify_sha1sum = false;
  bool dry_run = false;

  app.set_version_flag("--version", facebook::BuildInfo::toDebugString());
  app.add_option(
      "--fw_target_name",
      fw_target_name,
      "The fpd name that needs to be programmed");
  app.add_option(
      "--fw_action",
      fw_action,
      "The firmware action (program, verify, read, version, list, audit)");
  app.add_option(
      "--fw_binary_file",
      fw_binary_file,
      "The binary file to program (only required for 'program')");

  app.add_flag(
      "--verify_sha1sum", verify_sha1sum, "Verify SHA1 sum of the firmware");
  app.add_flag(
      "--dry_run", dry_run, "Dry run without actually programming firmware");

  CLI11_PARSE(app, argc, argv);

  // TODO: To be removed once XFN change the commands in their codes
  if (fw_action.empty()) {
    if (argc > 1) {
      fw_target_name = argv[1];
    }
    if (argc > 2) {
      fw_action = argv[2];
    }
    if (argc > 3) {
      fw_binary_file = argv[3];
    }
    XLOG(WARNING)
        << "Deprecation Warning: This current command line format will soon be deprecated.Please consider using fw_util --fw_target_name='binaryName' --fw_action='action' --fw_binary_file='path_to_file_to_be_upraded'";
  }

  if (fw_action == "version" && !fw_binary_file.empty()) {
    XLOG(ERR) << "--fw_binary_file cannot be part of version command";
    exit(1);
  }

  if (fw_action == "program" &&
      (fw_target_name.empty() || fw_binary_file.empty())) {
    XLOG(ERR)
        << "--fw_binary_file and --fw_target_name need to be specified for firmware programming";
    exit(1);
  }

  // Check for file existence only for actions that require a binary file
  if (!fw_binary_file.empty() && fw_action != "read" &&
      !std::filesystem::exists(fw_binary_file)) {
    XLOG(ERR) << "--fw_binary_file cannot be found in the specified path";
    exit(1);
  }

  FwUtilImpl fwUtilImpl(fw_binary_file, verify_sha1sum, dry_run);

  if (fw_action == "version" && !fw_target_name.empty()) {
    fwUtilImpl.printVersion(toLower(fw_target_name));
  } else if (
      fw_action == "program" || fw_action == "verify" || fw_action == "read") {
    // For actions which involve more than just reading versions/config, we want
    // to always log all the commands that are run.
    folly::LoggerDB::get()
        .getCategory("fboss.platform.helpers.PlatformUtils")
        ->setLevel(folly::LogLevel::DBG2);
    fwUtilImpl.doFirmwareAction(toLower(fw_target_name), toLower(fw_action));
  } else if (fw_action == "list") {
    XLOG(INFO) << "supported Binary names are: " << fwUtilImpl.printFpdList();
  } else if (fw_action == "audit") {
    fwUtilImpl.doVersionAudit();
  } else {
    XLOG(ERR)
        << "Wrong usage. please run fw_util --helpon=Flags for the flags needed for proper usage";
    exit(1);
  }

  return 0;
}
