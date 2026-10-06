/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace facebook::fboss::platform::platform_checks {

struct CommandResult {
  int exitCode{-1};
  std::string standardOut;
  std::string standardErr;

  bool ok() const {
    return exitCode == 0;
  }
};

/**
 * A machine that checks run against: the local system, or a remote x86 or BMC.
 * Checks must perform all I/O through a Host so that the same check works both
 * on-device and remotely.
 */
class Host {
 public:
  static constexpr std::chrono::seconds kDefaultTimeout{30};

  virtual ~Host() = default;

  virtual std::string name() const = 0;

  virtual bool isLocal() const = 0;

  // Runs `cmd` with /bin/sh. A timed out command has exit code 124.
  virtual CommandResult run(
      const std::string& cmd,
      std::chrono::seconds timeout = kDefaultTimeout) const = 0;

  // Copies a local file or directory (recursively) to `hostPath` on this host.
  // Throws std::runtime_error on failure.
  virtual void copyTo(
      const std::filesystem::path& localPath,
      const std::filesystem::path& hostPath,
      std::chrono::seconds timeout) const = 0;

  // The defaults below are implemented with run(), which works on any host.

  virtual std::optional<std::string> readFile(
      const std::filesystem::path& path) const;

  virtual bool exists(const std::filesystem::path& path) const;

  // Returns full paths of the entries of `path`, or an empty list on failure.
  virtual std::vector<std::filesystem::path> listDirectory(
      const std::filesystem::path& path) const;
};

// Runs `argv` locally, killing it after `timeout`. Shared by Host
// implementations, which all end up spawning a local process.
CommandResult runLocalCommand(
    const std::vector<std::string>& argv,
    std::chrono::seconds timeout);

} // namespace facebook::fboss::platform::platform_checks
