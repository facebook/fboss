// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/PlatformUtils.h"

#include <folly/Subprocess.h>
#include <folly/logging/xlog.h>
#include <folly/system/Shell.h>

namespace fs = std::filesystem;
using namespace folly::literals::shell_literals;

namespace facebook::fboss::platform {

std::pair<int, std::string> PlatformUtils::execCommand(
    const std::string& cmd) const {
  XLOG(DBG2) << "Executing command: " << cmd;
  auto shellCmd = "/bin/sh -c {}"_shellify(cmd);
  folly::Subprocess p(shellCmd, folly::Subprocess::Options().pipeStdout());
  auto [standardOut, standardErr] = p.communicate();
  int exitStatus = p.wait().exitStatus();
  return std::make_pair(exitStatus, standardOut);
}

std::pair<int, std::string> PlatformUtils::runCommand(
    const std::vector<std::string>& args,
    bool printCommand) const {
  if (printCommand) {
    XLOG(INFO) << "Running command: " << folly::join(" ", args);
  }
  folly::Subprocess p(args, folly::Subprocess::Options().pipeStdout());
  auto [standardOut, standardErr] = p.communicate();
  int exitStatus = p.wait().exitStatus();
  return std::make_pair(exitStatus, standardOut);
}

std::pair<int, std::string> PlatformUtils::runCommandWithStdin(
    const std::vector<std::string>& cmd,
    const std::string& input,
    bool printCommand) const {
  if (printCommand) {
    XLOG(INFO) << "Running command: " << folly::join(" ", cmd);
  }
  folly::Subprocess::Options options;
  options.pipeStdin();
  options.pipeStdout();
  folly::Subprocess proc(cmd, options);
  auto [output1, output2] = proc.communicate(input);
  int exitStatus = proc.wait().exitStatus();
  return std::make_pair(exitStatus, output1);
}

} // namespace facebook::fboss::platform
