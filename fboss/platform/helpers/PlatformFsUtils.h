// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <folly/portability/Fcntl.h>

namespace facebook::fboss::platform {

class PlatformFsUtils {
 public:
  // If rootDir is provided, ALL file operations will be relative to it. In
  // particular, even absolute paths will be treated as relative to rootDir.
  // Example:
  //   PlatformFsUtils("/tmp").writeStringToFile("foo", "/tmp/bar")
  // will write "foo" to /tmp/tmp/bar, not /tmp/bar.
  explicit PlatformFsUtils(
      const std::optional<std::filesystem::path>& rootDir = std::nullopt);
  virtual ~PlatformFsUtils() = default;

  // Recursively create directories for the given path.
  // - for given /x/y/z, directory y/z if x already exists.
  // No-op if parent directories already exist.
  // Returns true if created or already exist, otherwise false.
  bool createDirectories(const std::filesystem::path& path) const;

  // Write string to file. Returns true if successful, otherwise false. File and
  // directories will be created (recursively) if they don't exist. By default,
  // if file already exists, it will be overwritten.
  virtual bool writeStringToFile(
      const std::string& content,
      const std::filesystem::path& path,
      int flags = O_WRONLY | O_CREAT | O_TRUNC) const;

  virtual std::optional<std::string> getStringFileContent(
      const std::filesystem::path& path) const;

 private:
  std::filesystem::path rootDir_;
};

} // namespace facebook::fboss::platform
