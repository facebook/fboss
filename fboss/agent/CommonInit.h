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

#include <csignal>
#include <map>
#include <memory>
#include <string>

#include <folly/io/async/AsyncSignalHandler.h>

namespace facebook::fboss {

struct AgentConfig;

void initFlagDefaults(const std::map<std::string, std::string>& defaults);
std::unique_ptr<AgentConfig> parseConfig(int argc, char** argv);
void fbossFinalize();
void setVersionInfo(const std::string& version);
void initializeBitsflow();
std::unique_ptr<AgentConfig> fbossCommonInit(int argc, char** argv);

class SignalHandler : public folly::AsyncSignalHandler {
 protected:
  using StopServices = std::function<void()>;

  void stopServices() {
    stopServices_();
  }

 public:
  SignalHandler(folly::EventBase* eventBase, StopServices stopServices)
      : AsyncSignalHandler(eventBase), stopServices_(stopServices) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }

 private:
  StopServices stopServices_;
};

} // namespace facebook::fboss
