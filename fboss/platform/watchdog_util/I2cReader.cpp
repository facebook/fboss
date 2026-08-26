// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/I2cReader.h"

#include <filesystem>

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::watchdog_util {

std::optional<uint8_t>
I2cReader::read(int bus, int addr, uint8_t reg, std::string& err) {
  // Determine i2cget binary path: try mock first for testing, then system
  std::string i2cgetBin = "i2cget";
  if (std::filesystem::exists("/tmp/mock_bin/i2cget")) {
    i2cgetBin = "/tmp/mock_bin/i2cget";
  } else if (std::filesystem::exists("/usr/sbin/i2cget")) {
    i2cgetBin = "/usr/sbin/i2cget";
  } else if (std::filesystem::exists("/usr/bin/i2cget")) {
    i2cgetBin = "/usr/bin/i2cget";
  }

  std::vector<std::string> args = {
      i2cgetBin,
      "-y",
      "-f",
      fmt::format("{}", bus),
      fmt::format("0x{:x}", addr),
      fmt::format("0x{:x}", reg)};

  XLOG(DBG2) << "Running: " << folly::join(" ", args);

  try {
    folly::Subprocess proc(
        args, folly::Subprocess::Options().pipeStdout().pipeStderr());
    auto [out, errOut] = proc.communicate();
    int exitCode = proc.wait().exitStatus();

    if (exitCode != 0) {
      err = fmt::format(
          "i2cget failed exit={} stderr={} stdout={}", exitCode, errOut, out);
      return std::nullopt;
    }

    std::string trimmed = folly::trimWhitespace(out).str();
    if (trimmed.empty()) {
      err = "i2cget empty output";
      return std::nullopt;
    }

    try {
      unsigned long val = std::stoul(trimmed, nullptr, 0);
      return static_cast<uint8_t>(val & 0xFF);
    } catch (const std::exception& ex) {
      err = fmt::format(
          "Failed to parse i2cget output '{}': {}", trimmed, ex.what());
      return std::nullopt;
    }
  } catch (const std::exception& ex) {
    err = fmt::format("Exception running i2cget: {}", ex.what());
    return std::nullopt;
  }
}

} // namespace facebook::fboss::platform::watchdog_util
