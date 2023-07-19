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
  virtual ~HwAgent() {}
  Platform* getPlatform() const {
    return platform_.get();
  }
  HwInitResult initAgent(
      bool failHwCallsOnWarmboot,
      HwSwitchCallback* callback);
  void start();

 private:
  std::unique_ptr<Platform> platform_;
};
} // namespace facebook::fboss
