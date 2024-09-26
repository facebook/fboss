// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <folly/portability/Fcntl.h>

namespace facebook::fboss::platform {

// This class provides a set of file system utility functions used by platform
// services. It supports testing on the actual filesystem via rootDir field,
// which can be set to a temporary directory, allowing tests to run even when
// the tested code path uses absolute file paths. Users of this class should
// rely on it for all file operations which read/write to/from the filesystem to
// ensure consistent behavior.
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
  virtual bool createDirectories(const std::filesystem::path& path) const;

  bool exists(const std::filesystem::path& path) const;

  std::filesystem::directory_iterator ls(
      const std::filesystem::path& path) const;

  // Write string to file. Returns true if successful. File and directories will
  // be created (recursively) if they don't exist. By default, if file already
  // exists, it will be overwritten. File flags can be specified via `flags`
  // parameter.
  //
  // If `atomic` is true, the file will be written atomically, i.e. there will
  // never be partial writes. However, the write is NOT guaranteed durable, i.e.
  // the write may not be persisted after this function returns `true`.
  //
  // Note: `flags` are ignored when `atomic` = true.
  virtual bool writeStringToFile(
      const std::string& content,
      const std::filesystem::path& path,
      bool atomic = false,
      int flags = O_WRONLY | O_CREAT | O_TRUNC) const;

  virtual std::optional<std::string> getStringFileContent(
      const std::filesystem::path& path) const;

 private:
  std::filesystem::path rootDir_;
};

} // namespace facebook::fboss::platform
