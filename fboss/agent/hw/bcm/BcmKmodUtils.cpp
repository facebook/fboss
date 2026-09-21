// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmKmodUtils.h"

#include <folly/Subprocess.h>
#include <folly/logging/xlog.h>
#include <folly/system/Shell.h>

namespace facebook::fboss {

using folly::literals::shell_literals::operator""_shellify;

std::string getBcmKmodParam(const std::string& param) {
  folly::Subprocess p(
      "cat /sys/module/linux_user_bde/parameters/{}"_shellify(param),
      folly::Subprocess::Options().pipeStdout());
  auto [stdOut, stdErr] = p.communicate();
  int exitStatus = p.wait().exitStatus();
  if (exitStatus != 0 || stdOut.empty()) {
    XLOG(ERR) << "Failed in getting kmod pamameter. Error: " << stdErr;
  }
  return stdOut;
}

bool setBcmKmodParam(const std::string& param, const std::string& val) {
  folly::Subprocess p(
      "echo {} > /sys/module/linux_user_bde/parameters/{}"_shellify(val, param),
      folly::Subprocess::Options().pipeStdout());
  auto [_, stdErr] = p.communicate();
  int exitStatus = p.wait().exitStatus();
  if (exitStatus != 0) {
    XLOG(ERR) << "Failed in setting kmod pamameter. Error: " << stdErr;
    return false;
  }
  return true;
}

} // namespace facebook::fboss
