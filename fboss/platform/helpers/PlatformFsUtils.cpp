// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/PlatformFsUtils.h"

#include <filesystem>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace fs = std::filesystem;

namespace facebook::fboss::platform {

bool PlatformFsUtils::createDirectories(const std::string& path) {
  std::error_code errCode;
  std::filesystem::create_directories(fs::path(path), errCode);
  if (errCode.value() != 0) {
    XLOG(ERR) << fmt::format(
        "Received error code {} from creating path {}", errCode.value(), path);
  }
  return errCode.value() == 0;
}

std::optional<std::string> PlatformFsUtils::getStringFileContent(
    const std::string& path) const {
  std::string value{};
  if (!folly::readFile(path.c_str(), value)) {
    return std::nullopt;
  }
  return folly::trimWhitespace(value).str();
}

} // namespace facebook::fboss::platform
