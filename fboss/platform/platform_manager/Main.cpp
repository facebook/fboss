// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include <cstdlib>
#include <stdexcept>

#include <fb303/FollyLoggingHandler.h>
#include <fb303/ServiceData.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/platform_manager/PkgManager.h"
#include "fboss/platform/platform_manager/PlatformManagerHandler.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

DEFINE_int32(thrift_port, 5975, "Port for the thrift service");

DEFINE_int32(
    explore_interval_s,
    60,
    "Frequency at which the platform needs to be explored");

DEFINE_bool(
    run_once,
    true,
    "Setup platform once and exit. If set to false, setup platform once "
    "and run thrift service.");

void sdNotifyReady() {
  auto cmd = "systemd-notify --ready";
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error(fmt::format(
        "Failed to sd_notify ready by run command ({}). ExitStatus: {}",
        cmd,
        exitStatus));
  }
  XLOG(INFO) << fmt::format(
      "Sent sd_notify ready by running command ({})", cmd);
}

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(&argc, &argv);

  auto config = Utils().getConfig();

  PkgManager pkgManager(config);
  pkgManager.processAll();
  PlatformExplorer platformExplorer(config);
  platformExplorer.explore();
  if (FLAGS_run_once) {
    XLOG(INFO) << fmt::format(
        "Ran with --run_once={}. Skipping running as a daemon...",
        FLAGS_run_once);
    return 0;
  }

  // When systemd starts PlatformManager, it sets the below env in PM
  // environment. This is a path to Unix domain socket at /run/systemd/notify.
  // Ideally, we can use sd_notify in systemd/sd-daemon.h since it does the
  // check for us, if we can get OSS build to work.
  const auto notifySocketEnv{"NOTIFY_SOCKET"};
  if (std::getenv(notifySocketEnv)) {
    sdNotifyReady();
  } else {
    XLOG(WARNING) << fmt::format(
        "Skipping sd_notify since ${} is not set which does not "
        "imply systemd execution.",
        notifySocketEnv);
  }

  XLOG(INFO) << "Running PlatformManager thrift service...";

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<PlatformManagerHandler>(platformExplorer);
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(
      server, handler, "PlatformManagerService", FLAGS_thrift_port);
}
