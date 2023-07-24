// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/Utils.h"

#include <folly/Subprocess.h>
#include <folly/system/Shell.h>

using namespace folly::literals::shell_literals;
namespace {
std::string execCommandImpl(const std::string& cmd, int* exitStatus) {
  auto shellCmd = "/bin/sh -c {}"_shellify(cmd);
  folly::Subprocess p(shellCmd, folly::Subprocess::Options().pipeStdout());
  auto result = p.communicate();
  if (exitStatus) {
    *exitStatus = p.wait().exitStatus();
  } else {
    p.waitChecked();
  }
  return result.first;
}

} // namespace
namespace facebook::fboss::platform::helpers {

std::string execCommandUnchecked(const std::string& cmd, int& exitStatus) {
  return execCommandImpl(cmd, &exitStatus);
}

} // namespace facebook::fboss::platform::helpers
