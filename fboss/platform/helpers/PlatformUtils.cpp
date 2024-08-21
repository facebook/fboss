// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/PlatformUtils.h"

#include <filesystem>

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

bool PlatformUtils::createDirectories(const std::string& path) {
  std::error_code errCode;
  std::filesystem::create_directories(fs::path(path), errCode);
  if (errCode.value() != 0) {
    XLOG(ERR) << fmt::format(
        "Received error code {} from creating path {}", errCode.value(), path);
  }
  return errCode.value() == 0;
}

std::optional<std::string> PlatformUtils::getStringFileContent(
    const std::string& path) const {
  std::string value{};
  if (!folly::readFile(path.c_str(), value)) {
    return std::nullopt;
  }
  return folly::trimWhitespace(value).str();
}

} // namespace facebook::fboss::platform
