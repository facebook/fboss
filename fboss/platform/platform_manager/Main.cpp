// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include <stdexcept>

#include <fb303/FollyLoggingHandler.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/platform_manager/PkgUtils.h"
#include "fboss/platform/platform_manager/PlatformExplorer.h"
#include "fboss/platform/platform_manager/PlatformManagerHandler.h"
#include "fboss/platform/platform_manager/Utils.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

DEFINE_int32(thrift_port, 5975, "Port for the thrift service");

DEFINE_int32(
    explore_interval_s,
    60,
    "Frequency at which the platform needs to be explored");

DEFINE_bool(
    enable_pkg_mgmnt,
    true,
    "Enable download and installation of the BSP rpm");

DEFINE_bool(
    reload_kmods,
    false,
    "The kmods are usually reloaded only when the BSP RPM changes. "
    "But if this flag is set, the kmods will be reloaded everytime.");

DEFINE_bool(
    run_once,
    true,
    "Setup platform once and exit. If set to false, setup platform once "
    "and run thrift service.");

DEFINE_string(
    local_rpm_path,
    "",
    "Path to the local rpm file that needs to be installed on the system.");

void sdNotifyReady() {
  auto cmd = "systemd-notify --ready";
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error(
        fmt::format("Failed to sd_notify ready by run command ({}).", cmd));
  }
  XLOG(INFO) << fmt::format(
      "Sent sd_notify ready by running command ({})", cmd);
}

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(&argc, &argv);

  auto config = Utils().getConfig();

  PkgUtils().loadUpstreamKmods(config);

  if (FLAGS_enable_pkg_mgmnt) {
    if (FLAGS_local_rpm_path != "") {
      PkgUtils().processLocalRpms(FLAGS_local_rpm_path, config);
    } else {
      PkgUtils().processRpms(config);
    }
  }

  if (FLAGS_reload_kmods) {
    PkgUtils().processKmods(config);
  }

  PlatformExplorer platformExplorer(config);
  platformExplorer.explore();
  if (FLAGS_run_once) {
    XLOG(INFO) << fmt::format(
        "Ran with --run_once={}. Skipping running as a daemon...",
        FLAGS_run_once);
    return 0;
  }
  sdNotifyReady();

  XLOG(INFO) << "Running PlatformManager thrift service...";

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<PlatformManagerHandler>();
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(
      server, handler, "PlatformManagerService", FLAGS_thrift_port);
}
