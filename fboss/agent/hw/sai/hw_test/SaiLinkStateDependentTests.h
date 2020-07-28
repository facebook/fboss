// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook {
namespace fboss {

class SaiLinkStateDependentTests : public HwLinkStateDependentTest {
 public:
  SaiLinkStateDependentTests() {}
  ~SaiLinkStateDependentTests() {}

  SaiSwitch* getSaiSwitch() {
    return static_cast<SaiSwitch*>(HwTest::getHwSwitch());
  }
  SaiPlatform* getSaiPlatform() {
    return static_cast<SaiPlatform*>(HwTest::getPlatform());
  }
  const SaiSwitch* getSaiSwitch() const {
    return static_cast<const SaiSwitch*>(HwTest::getHwSwitch());
  }
  const SaiPlatform* getSaiPlatform() const {
    return static_cast<const SaiPlatform*>(HwTest::getPlatform());
  }

 private:
  // Forbidden copy constructor and assignment operator
  SaiLinkStateDependentTests(SaiLinkStateDependentTests const&) = delete;
  SaiLinkStateDependentTests& operator=(SaiLinkStateDependentTests const&) =
      delete;
};

} // namespace fboss
} // namespace facebook
