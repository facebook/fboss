/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <folly/logging/xlog.h>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/MockTunManager.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

using ::testing::_;

TEST(TunInterfacesTest, Initialization) {
  auto platform = createMockPlatform();
  auto sw = setupMockSwitchWithoutHW(
      platform.get(), nullptr, SwitchFlags::ENABLE_TUN);
  auto tunMgr = dynamic_cast<MockTunManager*>(sw->getTunManager());
  EXPECT_NE(nullptr, tunMgr);
  EXPECT_CALL(*tunMgr, sync(_)).Times(1);
  EXPECT_CALL(*tunMgr, startObservingUpdates()).Times(1);
  sw->initialConfigApplied(std::chrono::steady_clock::now());
  // initial sync for TunManager is asyc and happens on the event base
  // passed to Tun manager. For MockTunManager this is always the background
  // event base. So wait for pending operations there to complete
  waitForBackgroundThread(sw.get());
}
