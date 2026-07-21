// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <cstdio>
#include <filesystem>

#include <gflags/gflags.h>

#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/watchdog_util/WatchdogUtil.h"

#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

DEFINE_string(devmap_dir, "/run/devmap/watchdogs", "Watchdog devmap directory");
DEFINE_string(pm_config_file, "", "Optional platform_manager.json path");
DEFINE_bool(json, false, "Output in JSON format");
DEFINE_bool(verbose, false, "Verbose logging");

using namespace facebook::fboss::platform::watchdog_util;

int main(int argc, char* argv[]) {
  facebook::fboss::platform::helpers::initCli(&argc, &argv, "watchdog_util");

  if (FLAGS_verbose) {
    folly::LoggerDB::get().setLevel("fboss", folly::LogLevel::DBG);
  }

  XLOG(INFO) << "Starting watchdog_util devmap_dir=" << FLAGS_devmap_dir
             << " pm_config_file=" << FLAGS_pm_config_file;

  // Per requirement: exit after devmap dir missing error, do not continue.
  // Use a distinct exit code (2) so wrappers/monitoring can tell "no devmap
  // dir" apart from exit code 1, which means "some watchdogs failed to read".
  if (!std::filesystem::exists(FLAGS_devmap_dir)) {
    fprintf(
        stderr, "Devmap dir does not exist: %s\n", FLAGS_devmap_dir.c_str());
    XLOG(WARN) << "Devmap dir does not exist: " << FLAGS_devmap_dir;
    return 2;
  }

  WatchdogUtil util(FLAGS_devmap_dir, FLAGS_pm_config_file);
  auto infos = util.getAllWatchdogInfo();

  if (FLAGS_json) {
    WatchdogUtil::printJson(infos);
  } else {
    WatchdogUtil::printHuman(infos, FLAGS_devmap_dir);
  }

  // Return non-zero if any watchdog failed to read
  bool anyError = false;
  for (const auto& info : infos) {
    if (!info.valid) {
      anyError = true;
      break;
    }
  }

  return anyError ? 1 : 0;
}
