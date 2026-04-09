/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <string>
#include <utility>

#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

// Test-only derived class that exposes the protected constructor and methods
// This allows tests to inject custom paths and control the singleton instance
class TestableConfigSession : public ConfigSession {
 public:
  TestableConfigSession(
      std::string sessionConfigDir,
      std::string systemConfigDir)
      : ConfigSession(std::move(sessionConfigDir), std::move(systemConfigDir)) {
  }

  // Constructor with mock FbossServiceUtil
  TestableConfigSession(
      std::string sessionConfigDir,
      std::string systemConfigDir,
      std::unique_ptr<FbossServiceUtil> fbossServiceUtil)
      : ConfigSession(
            std::move(sessionConfigDir),
            std::move(systemConfigDir),
            std::move(fbossServiceUtil)) {}

  // Expose protected setInstance() for testing
  using ConfigSession::setInstance;

  // Expose protected applyServiceActions() for testing
  using ConfigSession::applyServiceActions;

  // Set the command line to return from readCommandLineFromProc().
  // This allows tests to simulate CLI commands without /proc/self/cmdline.
  void setCommandLine(const std::string& commandLine) {
    commandLine_ = commandLine;
  }

  // Override to return the mock command line set by setCommandLine().
  std::string readCommandLineFromProc() const override {
    return commandLine_;
  }

 private:
  std::string commandLine_;
};

} // namespace facebook::fboss
