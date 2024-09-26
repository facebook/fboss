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
        "Received error code {} from creating path {}: {}",
        errCode.value(),
        path.string(),
        errCode.message());
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
    bool atomic,
    int flags) const {
  const fs::path& prefixedPath = concat(rootDir_, path);
  if (!createDirectories(path.parent_path())) {
    return false;
  }
  int errorCode = 0;
  if (atomic) {
    auto options = folly::WriteFileAtomicOptions();
    options.setSyncType(folly::SyncType::WITH_SYNC);
    errorCode = folly::writeFileAtomicNoThrow(
        folly::StringPiece(prefixedPath.c_str()), content, options);
  } else {
    bool written = folly::writeFile(content, prefixedPath.c_str(), flags);
    // errno will be set to 2 when the file did not exist but was successfully
    // created & written. Only set errCode on failed writes.
    if (!written) {
      errorCode = errno;
    }
  }
  if (errorCode != 0) {
    XLOG(ERR) << fmt::format(
        "Received error code {} from writing to path {}: {}",
        errorCode,
        path.string(),
        folly::errnoStr(errorCode));
  }
  return errorCode == 0;
}

std::optional<std::string> PlatformFsUtils::getStringFileContent(
    const fs::path& path) const {
  std::string value{};
  if (!folly::readFile(concat(rootDir_, path).c_str(), value)) {
    int errorCode = errno;
    XLOG(ERR) << fmt::format(
        "Received error code {} from reading from path {}: {}",
        errorCode,
        path.string(),
        folly::errnoStr(errorCode));
    return std::nullopt;
  }
  return folly::trimWhitespace(value).str();
}

} // namespace facebook::fboss::platform
