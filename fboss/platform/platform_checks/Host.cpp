/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/platform_checks/Host.h"

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <folly/logging/xlog.h>
#include <folly/system/Shell.h>

namespace facebook::fboss::platform::platform_checks {

CommandResult runLocalCommand(
    const std::vector<std::string>& argv,
    std::chrono::seconds timeout) {
  // coreutils `timeout` is simpler than folly::Subprocess polling and gives the
  // conventional exit code 124 on expiry.
  std::vector<std::string> timedArgv{
      "timeout", "--kill-after=5", std::to_string(timeout.count())};
  timedArgv.insert(timedArgv.end(), argv.begin(), argv.end());
  XLOG(DBG2) << "Running: " << folly::join(" ", timedArgv);

  CommandResult result;
  try {
    folly::Subprocess proc(
        timedArgv,
        folly::Subprocess::Options().pipeStdout().pipeStderr().usePath());
    std::tie(result.standardOut, result.standardErr) = proc.communicate();
    auto returnCode = proc.wait();
    result.exitCode = returnCode.exited() ? returnCode.exitStatus()
                                          : 128 + returnCode.killSignal();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to run " << folly::join(" ", timedArgv) << ": "
              << ex.what();
    result.exitCode = -1;
    result.standardErr = ex.what();
  }
  return result;
}

std::optional<std::string> Host::readFile(
    const std::filesystem::path& path) const {
  auto result = run("cat -- " + folly::shellQuote(path.string()));
  if (!result.ok()) {
    return std::nullopt;
  }
  return result.standardOut;
}

bool Host::exists(const std::filesystem::path& path) const {
  return run("test -e " + folly::shellQuote(path.string())).ok();
}

std::optional<std::string> Host::readTrimmedFile(
    const std::filesystem::path& path) const {
  auto content = readFile(path);
  if (!content) {
    return std::nullopt;
  }
  return folly::trimWhitespace(*content).str();
}

std::vector<std::filesystem::path> Host::listDirectory(
    const std::filesystem::path& path) const {
  std::vector<std::filesystem::path> entries;
  // NUL-separated, since file names may contain newlines. A plain glob loop
  // rather than `find -print0`, which BusyBox (BMCs) may not support.
  auto result =
      run("cd -- " + folly::shellQuote(path.string()) +
          " || exit 1; for f in * .[!.]* ..?*; do "
          "if [ -e \"$f\" ] || [ -L \"$f\" ]; then printf '%s\\0' \"$f\"; fi; "
          "done");
  if (!result.ok()) {
    return entries;
  }
  std::vector<folly::StringPiece> names;
  folly::split('\0', result.standardOut, names, /*ignoreEmpty=*/true);
  for (const auto& entryName : names) {
    entries.push_back(path / entryName.str());
  }
  return entries;
}

} // namespace facebook::fboss::platform::platform_checks
