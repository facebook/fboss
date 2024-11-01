/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/TestUtils.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/lib/CommonFileUtils.h"

#include <folly/logging/xlog.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace {
auto constexpr kConfigFileWaitPeriod = std::chrono::milliseconds{100};
}

namespace facebook::fboss {
std::unique_ptr<AgentConfig> getConfigFileForTesting(int switchIndex) {
  auto configFileName =
      AgentDirectoryUtil().getTestHwAgentConfigFile(switchIndex);
  std::condition_variable configFileCv;
  std::mutex configFileMutex;
  std::unique_lock<std::mutex> lock(configFileMutex);
  XLOG(INFO) << "Waiting on config file " << configFileName
             << " to init hw agent";
  while (!checkFileExists(configFileName)) {
    configFileCv.wait_for(lock, kConfigFileWaitPeriod, [&]() {
      return checkFileExists(configFileName);
    });
  }
  XLOG(INFO) << "Using config file " << configFileName << " to init hw agent";
  auto config = AgentConfig::fromFile(configFileName);
  removeFile(configFileName);
  return config;
}
} // namespace facebook::fboss
