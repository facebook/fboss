// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/PlatformFsUtils.h"

#include <filesystem>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace fs = std::filesystem;

namespace {

// Convenience function. We do not use path::append (/), because for (/), if the
// RHS is an absolute path, the LHS is just discarded, whereas we explicitly
// treat absolute paths as relative to rootDir_. We still introduce a path
// separator between p1 and p2 as we do not want "/foo" + "bar" = "/foobar".
fs::path concat(const fs::path& p1, const fs::path& p2) {
  if (p1.empty()) {
    return p2;
  }
  fs::path result = p1;
  // Ensure there's at least one path separator.
  result += fs::path::preferred_separator;
  result += p2;
  // Fix extra path separators.
  return result.lexically_normal();
}

} // namespace

namespace facebook::fboss::platform {

PlatformFsUtils::PlatformFsUtils(const std::optional<fs::path>& rootDir) {
  if (!rootDir.has_value()) {
    rootDir_ = "";
    return;
  }
  rootDir_ = rootDir.value();
}

bool PlatformFsUtils::createDirectories(const fs::path& path) const {
  std::error_code errCode;
  fs::create_directories(concat(rootDir_, path), errCode);
  if (errCode.value() != 0) {
    XLOG(ERR) << fmt::format(
        "Received error code {} from creating path {}",
        errCode.value(),
        path.string());
  }
  return errCode.value() == 0;
}

bool PlatformFsUtils::exists(const fs::path& path) const {
  return fs::exists(concat(rootDir_, path));
}

fs::directory_iterator PlatformFsUtils::ls(const fs::path& path) const {
  return fs::directory_iterator(concat(rootDir_, path));
}

bool PlatformFsUtils::writeStringToFile(
    const std::string& content,
    const fs::path& path,
    int flags) const {
  if (!createDirectories(path.parent_path())) {
    return false;
  }
  return folly::writeFile(content, concat(rootDir_, path).c_str(), flags);
}

std::optional<std::string> PlatformFsUtils::getStringFileContent(
    const fs::path& path) const {
  std::string value{};
  if (!folly::readFile(concat(rootDir_, path).c_str(), value)) {
    return std::nullopt;
  }
  return folly::trimWhitespace(value).str();
}

} // namespace facebook::fboss::platform
