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

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/session/SystemdInterface.h"

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

  void setMultiSwitchOverride(
      bool multiSwitch,
      std::vector<int> switchIndexes = {0}) {
    multiSwitchOverride_ = multiSwitch;
    switchIndexesOverride_ = std::move(switchIndexes);
  }

  void setMockSystemdFactory(
      std::function<std::unique_ptr<SystemdInterface>()> factory) {
    mockSystemdFactory_ = std::move(factory);
  }

  void ensureFbossServiceUtil(const HostInfo& /*hostInfo*/) override {
    if (!fbossServiceUtil_) {
      if (mockSystemdFactory_) {
        fbossServiceUtil_ = std::make_unique<FbossServiceUtil>(
            switchIndexesOverride_,
            multiSwitchOverride_,
            mockSystemdFactory_());
      } else {
        fbossServiceUtil_ = std::make_unique<FbossServiceUtil>(
            switchIndexesOverride_, multiSwitchOverride_);
      }
    }
  }

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
  bool multiSwitchOverride_{false};
  std::vector<int> switchIndexesOverride_{0};
  std::function<std::unique_ptr<SystemdInterface>()> mockSystemdFactory_;
};

} // namespace facebook::fboss
