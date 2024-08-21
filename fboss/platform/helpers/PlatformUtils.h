// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <optional>
#include <string>

namespace facebook::fboss::platform {

class PlatformUtils {
 public:
  virtual ~PlatformUtils() = default;
  // Executes command and returns exit status and standard output
  virtual std::pair<int, std::string> execCommand(const std::string& cmd) const;

  // Recursively create directories for the given path.
  // - for given /x/y/z, directory y/z if x already exists.
  // No-op if parent directories already exist.
  // Returns true if created or already exist, otherwise false.
  bool createDirectories(const std::string& path);

  virtual std::optional<std::string> getStringFileContent(
      const std::string& path) const;
};

} // namespace facebook::fboss::platform
