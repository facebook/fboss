//  Copyright 2021-present Facebook. All Rights Reserved.

#include <sysexits.h>

#include <folly/logging/xlog.h>

#include "fboss/platform/fw_util/Flags.h"
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/InitCli.h"

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

  helpers::initCli(&argc, &argv, "fw_util");

  FwUtilImpl fwUtilImpl;

  // TODO: To be removed once XFN change the commands in their codes
  if (FLAGS_fw_action.empty()) {
    if (argc > 1) {
      FLAGS_fw_target_name = argv[1];
    }
    if (argc > 2) {
      FLAGS_fw_action = argv[2];
    }
    if (argc > 3) {
      FLAGS_fw_binary_file = argv[3];
    }
    XLOG(WARNING)
        << "Deprecation Warning: This current command line format will soon be deprecated.Please consider using fw_util --fw_target_name='binaryName' --fw_action='action' --fw_binary_file='path_to_file_to_be_upraded'";
  }

  if (FLAGS_fw_action == "version" && !FLAGS_fw_binary_file.empty()) {
    XLOG(ERR) << "--fw_binary_file cannot be part of version command";
    exit(1);
  }

  if (FLAGS_fw_action == "program" &&
      (FLAGS_fw_target_name.empty() || FLAGS_fw_binary_file.empty())) {
    XLOG(ERR)
        << "--fw_binary_file and --fw_target_name need to be specified for firmware programming";
    exit(1);
  }

  if (!FLAGS_fw_binary_file.empty() &&
      !fwUtilImpl.isFilePresent(FLAGS_fw_binary_file)) {
    XLOG(ERR) << "--fw_binary_file cannot be found in the specified path";
    exit(1);
  }

  if (FLAGS_fw_action == "version" && !FLAGS_fw_target_name.empty()) {
    fwUtilImpl.printVersion(fwUtilImpl.toLower(FLAGS_fw_target_name));
  } else if (
      FLAGS_fw_action == "program" || FLAGS_fw_action == "verify" ||
      FLAGS_fw_action == "read") {
    fwUtilImpl.storeFilePath(
        fwUtilImpl.toLower(FLAGS_fw_target_name), FLAGS_fw_binary_file);
    fwUtilImpl.doFirmwareAction(
        fwUtilImpl.toLower(FLAGS_fw_target_name),
        fwUtilImpl.toLower(FLAGS_fw_action));
    // remove store file path after programing
    fwUtilImpl.removeFilePath(fwUtilImpl.toLower(FLAGS_fw_target_name));
  } else if (FLAGS_fw_action == "list") {
    XLOG(INFO) << "supported Binary names are: " << fwUtilImpl.printFpdList();
  } else if (FLAGS_fw_action == "audit") {
    fwUtilImpl.doVersionAudit();
  } else {
    XLOG(ERR)
        << "Wrong usage. please run fw_util --helpon=Flags for the flags needed for proper usage";
    exit(1);
  }

  return 0;
}
