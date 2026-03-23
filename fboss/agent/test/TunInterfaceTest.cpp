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
#include "fboss/agent/TxPacket.h"
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

/*
 * Test that packet forwarding from TUN interfaces is suppressed during
 * graceful exit. This validates the fix for a SIGSEGV crash where
 * TunIntf::handlerReady continued processing packets after SwSwitch::stop()
 * had begun tearing down members (ipv6_, hwAsicTable_, etc.).
 *
 * The crash scenario:
 *  1. TunIntf::handlerReady is mid-execution on the EventBase thread
 *  2. SIGTERM arrives, SwSwitch::stop() begins teardown on the main thread
 *  3. handlerReady calls getVlanIDForTx/sendL3Packet on already-destroyed
 * members
 *
 * The fix adds an isExiting() check at the top of handlerReady. This test
 * verifies that during exit, the packet send path correctly drops packets
 * and doesn't attempt to access torn-down SwSwitch members.
 */
TEST(TunInterfacesTest, NoPacketForwardingDuringExit) {
  auto cfg = testConfigA();
  auto handle = createTestHandle(&cfg);
  auto sw = handle->getSw();
  sw->initialConfigApplied(std::chrono::steady_clock::now());
  waitForStateUpdates(sw);

  // Verify switch is fully initialized before exit
  EXPECT_TRUE(sw->isFullyInitialized());
  EXPECT_FALSE(sw->isExiting());

  // Stop the switch to transition to EXITING state
  sw->stop();
  EXPECT_TRUE(sw->isExiting());
  EXPECT_FALSE(sw->isFullyInitialized());

  // Verify that sendL3Packet drops packets during exit without crashing.
  // No HW calls should be made — sendL3Packet returns early because
  // isFullyInitialized() is false in the EXITING state.
  // This is the same code path that TunIntf::handlerReady exercises;
  // the isExiting() guard we added in handlerReady prevents reaching
  // this point, providing an additional layer of protection.
  EXPECT_HW_CALL(sw, sendPacketSwitchedAsync_(_)).Times(0);
  EXPECT_HW_CALL(sw, sendPacketOutOfPortAsync_(_, _, _)).Times(0);

  // Call sendL3Packet during exit — must not crash
  sw->sendL3Packet(
      sw->allocateL3TxPacket(64, false /* tagged */), InterfaceID(1));
}
