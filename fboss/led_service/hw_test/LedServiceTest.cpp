/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "common/init/Init.h"
#include "fboss/led_service/LedManager.h"
#include "fboss/led_service/hw_test/LedServiceTest.h"

namespace facebook::fboss {

void LedServiceTest::SetUp() {
  // Create ensemble and initialize it
  ensemble_ = std::make_unique<LedEnsemble>();
  ensemble_->init();

  // Check if the LED manager is created correctly, the config file is loaded
  // and the LED is managed by service now
  auto ledManager = getLedEnsemble()->getLedManager();
  CHECK_NE(ledManager, nullptr);
  CHECK(ledManager->isLedControlledThroughService());
}

void LedServiceTest::TearDown() {
  // Remove ensemble and objects contained there
  ensemble_.reset();
}

} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::initFacebook(&argc, &argv);

  return 0;
}
