// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
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
    run_once,
    true,
    "Setup platform once and exit. If set to false, the program will explore "
    "the platform every explore_interval_s.");

DEFINE_string(
    local_rpm_path,
    "",
    "Path to the local rpm file that needs to be installed on the system.");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(&argc, &argv);

  auto config = Utils().getConfig();

  if (FLAGS_enable_pkg_mgmnt) {
    if (FLAGS_local_rpm_path != "") {
      PkgUtils().processLocalRpms(FLAGS_local_rpm_path);
    } else {
      PkgUtils().processRpms(config);
    }
  }
  PkgUtils().processKmods(config);

  PlatformExplorer platformExplorer(
      std::chrono::seconds(FLAGS_explore_interval_s), config, FLAGS_run_once);

  // If it is a one time setup, we don't have to run the thrift service.
  if (FLAGS_run_once) {
    return 0;
  }

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<PlatformManagerHandler>();
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setSSLPolicy(apache::thrift::SSLPolicy::DISABLED);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(
      server, handler, "PlatformManagerService", FLAGS_thrift_port);

  return 0;
}
