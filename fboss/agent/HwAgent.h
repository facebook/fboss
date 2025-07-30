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
#include <memory>

#include <gflags/gflags.h>
#include <string>

#include "fboss/agent/FbossInit.h"
#include "fboss/agent/HwSwitch.h"

namespace facebook::fboss {

class HwAgent {
 public:
  HwAgent(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatformFn,
      int16_t switchIndex);
  explicit HwAgent(std::unique_ptr<Platform> platform);
  virtual ~HwAgent() {}
  Platform* getPlatform() {
    return platform_.get();
  }
  const Platform* getPlatform() const {
    return platform_.get();
  }
  HwInitResult initAgent(
      bool failHwCallsOnWarmboot,
      HwSwitchCallback* callback);
  /*
   * Stop all threads that are running. The function is virtual to allow
   * derived classes to stop additional threads. Currently, HwAgent stops the
   * Platform threads, particularly the WedgePlatform BcmSwitch for high
   * frequency stats collection.
   */
  virtual void stopThreads();

  void waitForInitDone();

  bool isInitialized() const;

 private:
  std::unique_ptr<Platform> platform_;
  folly::Baton<> inited_{};
};
} // namespace facebook::fboss
