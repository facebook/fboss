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

#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"

namespace facebook::fboss {

class BcmConfigSetupTest : public BcmLinkStateDependentTests {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return getConfig();
  }

  std::function<void(void)> testSetup() {
    // Pre-warmboot config is already applied in initialConfig
    return []() {};
  }

  std::function<void(void)> testVerify() {
    // Post warmboot, apply new config (in setupPostWb), and then verify (in
    // verifyPostWb). Thus, nothing to verify here.
    return []() {};
  }

  std::function<std::shared_ptr<SwitchState>(void)> testSetupPostWb() {
    // Post warmboot, apply new config
    return [this]() { return applyNewConfig(getConfig()); };
  }

 private:
  cfg::SwitchConfig getConfig() const;
  virtual cfg::SwitchConfig getFallbackConfig() const = 0;
  std::unique_ptr<AgentConfig> getAgentConfigFromFile() const;

  cfg::SwitchConfig setPortsToLoopback(
      std::unique_ptr<AgentConfig> agentCfg) const;
};

} // namespace facebook::fboss
