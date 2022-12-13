// Copyright 2021- Facebook. All rights reserved.

#include "fboss/platform/fan_service/SetupThrift.h"
#include <folly/Subprocess.h>
#include <folly/system/Shell.h>
#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/Main.h"

namespace facebook::fboss::platform {

void startServiceAndRunServer(
    std::string serviceName,
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler,
    bool loopForever) {}

// runServer : a helper function to run Fan Service as Thrift Server.
void runServer(
    facebook::services::ServiceFrameworkLight& /*service*/,
    std::shared_ptr<apache::thrift::ThriftServer> /*thriftServer*/,
    facebook::fboss::platform::FanServiceHandler* /*handler*/,
    bool /*loopForever*/) {}

} // namespace facebook::fboss::platform

int runShellCmd(const std::string& cmd) {
  std::vector<std::string> shellifiedCmd = {"/bin/sh", "-c", cmd};
  auto subProc = folly::Subprocess(
      std::move(shellifiedCmd), folly::Subprocess::Options().pipeStdout());
  auto rc = subProc.wait().exitStatus();
  return rc;
}
