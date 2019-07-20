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

#include "fboss/agent/hw/benchmarks/HwSwitchBenchmarker.h"

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"

namespace facebook {
namespace fboss {

class BcmBenchmarker : public HwSwitchBenchmarker {
 public:
  BcmTestPlatform* getPlatform() override {
    return static_cast<BcmTestPlatform*>(HwSwitchBenchmarker::getPlatform());
  }
  BcmSwitch* getHwSwitch() override {
    return static_cast<BcmSwitch*>(HwSwitchBenchmarker::getHwSwitch());
  }

 private:
  std::unique_ptr<HwSwitch> createHwSwitch(Platform* platform) override;
  std::unique_ptr<HwLinkStateToggler> createLinkToggler(
      HwSwitch* hwSwitch) override;
};

std::unique_ptr<BcmBenchmarker> setupBenchmarker();

} // namespace fboss
} // namespace facebook
