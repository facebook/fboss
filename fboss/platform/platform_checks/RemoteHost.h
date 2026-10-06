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

#include <string>
#include <vector>

#include "fboss/platform/platform_checks/Host.h"

namespace facebook::fboss::platform::platform_checks {

/**
 * A switch (x86 or BMC) reached over SSH. Requires non-interactive (key or
 * certificate based) authentication.
 */
class RemoteHost : public Host {
 public:
  enum class Transport {
    // OpenSSH ssh/scp.
    SSH,
    // Meta's sush2/suscp2 wrappers, which handle authentication to switches.
    SUSH2,
  };

  // SUSH2 when sush2 is on PATH, SSH otherwise.
  static Transport detectTransport();

  RemoteHost(
      std::string hostname,
      Transport transport,
      std::string user = "root");

  std::string name() const override {
    return hostname_;
  }

  bool isLocal() const override {
    return false;
  }

  CommandResult run(
      const std::string& cmd,
      std::chrono::seconds timeout = kDefaultTimeout) const override;

  void copyTo(
      const std::filesystem::path& localPath,
      const std::filesystem::path& hostPath,
      std::chrono::seconds timeout) const override;

  // Local argv that runs `cmd` on the host. Virtual for tests.
  virtual std::vector<std::string> execArgv(const std::string& cmd) const;

  std::vector<std::string> copyArgv(
      const std::filesystem::path& localPath,
      const std::filesystem::path& hostPath) const;

 private:
  std::string destination() const;

  std::string hostname_;
  Transport transport_;
  std::string user_;
};

} // namespace facebook::fboss::platform::platform_checks
