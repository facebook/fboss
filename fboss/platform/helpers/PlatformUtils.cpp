// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/PlatformUtils.h"

#include <folly/Subprocess.h>
#include <folly/system/Shell.h>

using namespace folly::literals::shell_literals;

namespace facebook::fboss::platform {

std::pair<int, std::string> PlatformUtils::execCommand(const std::string& cmd) {
  auto shellCmd = "/bin/sh -c {}"_shellify(cmd);
  folly::Subprocess p(shellCmd, folly::Subprocess::Options().pipeStdout());
  auto [standardOut, standardErr] = p.communicate();
  int exitStatus = p.wait().exitStatus();
  return std::make_pair(exitStatus, standardOut);
}

} // namespace facebook::fboss::platform
