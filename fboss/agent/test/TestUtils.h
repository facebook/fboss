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
#include <functional>
#include <gmock/gmock.h>

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/FbossError.h"

namespace facebook { namespace fboss {

class MockHwSwitch;
class MockPlatform;
class Platform;
class SwitchState;
class SwSwitch;
class TxPacket;

namespace cfg {
class SwitchConfig;
}

/*
 * In the non unit test code state passed to apply*Config is the state
 * returned from SwSwitch init, which is always published. However this
 * is not the case for unit test code where we are constructing config
 * by hand and applying it to a (often empty) initial state. Now in a rare
 * case if such a config had only route updates in it, since the previous
 * state was not published, we would apply these route updates inline
 * and now when we compare the old and new route tables to determine
 * if anything changed we would come up empty. The end result in such
 * a case would be to return a null new state falsely signifying no change.
 * To avoid this, provide convenience wrappers for unit test code to call.
 */
std::shared_ptr<SwitchState> publishAndApplyConfig(
    std::shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    const cfg::SwitchConfig* prevCfg=nullptr);

std::shared_ptr<SwitchState> publishAndApplyConfigFile(
    std::shared_ptr<SwitchState>& state,
    folly::StringPiece path,
    const Platform* platform,
    std::string prevCfgStr="");

/*
 * Create a SwSwitch for testing purposes, with the specified initial state.
 *
 * This will use a MockHwSwitch for the HwSwitch implementation, with the
 * specified initial state.  The returned SwSwitch will have already been
 * initialized.
 */
std::unique_ptr<SwSwitch> createMockSw(const std::shared_ptr<SwitchState>&);
std::unique_ptr<SwSwitch> createMockSw(const std::shared_ptr<SwitchState>&,
                                       const folly::MacAddress&);

std::unique_ptr<SwSwitch> createMockSw(cfg::SwitchConfig* cfg,
                                       folly::MacAddress mac,
                                       uint32_t maxPorts = 0);
std::unique_ptr<SwSwitch> createMockSw(cfg::SwitchConfig* cfg,
                                       uint32_t maxPorts = 0);

/*
 * Get the MockHwSwitch from a SwSwitch.
 */
MockHwSwitch* getMockHw(SwSwitch* sw);

/*
 * Get the MockPlatform from a SwSwitch.
 */
MockPlatform* getMockPlatform(SwSwitch* sw);

/*
 * Wait until all pending StateUpdates have been applied.
 */
void waitForStateUpdates(SwSwitch* sw);

/*
 * Wait until all pending actions on the background thread
 * have been processed
 */
void waitForBackgroundThread(SwSwitch* sw);

/**
 * check the field value
 */
template<typename ExpectedType, typename ActualType>
void checkField(const ExpectedType& expected, const ActualType& actual,
                folly::StringPiece fieldName) {
  if (expected != actual) {
    throw FbossError("expected ", fieldName, " to be ", expected,
                     ", but found ", actual);
    return;
  }
}

/*
 * Create a SwitchState for testing.
 *
 * Profile A:
 *   - 20 ports, 2 VLANS, 2 interfaces
 *   - Ports 1-10 are in VLAN 1
 *   - Ports 11-20 are in VLAN 55
 *   - Interface 1:
 *     - VLAN 1
 *     - MAC 00:02:00:00:00:01
 *     - IPs:
 *       10.0.0.1/24
 *       192.168.0.1/24
 *       2401:db00:2110:3001::0001/64
 *   - Interface 55:
 *     - VLAN 55
 *     - MAC 00:02:00:00:00:55
 *     - IPs:
 *       10.0.55.1/24
 *       192.168.55.1/24
 *       2401:db00:2110:3055::0001/64
 */
std::shared_ptr<SwitchState> testStateA();

/*
 * Create a SwitchState for testing.
 *
 * Profile B:
 *   - 10 ports, 1 VLAN, 1 interface
 *   - Ports 1-10 are in VLAN 1
 *   - Interface 1:
 *     - VLAN 1
 *     - MAC 00:02:00:00:00:01
 *     - IPs:
 *       10.0.0.1/24
 *       192.168.0.1/24
 *       2401:db00:2110:3001::0001/64
 */
std::shared_ptr<SwitchState> testStateB();

/*
 * Convenience macro that wraps EXPECT_CALL() on the underlying MockHwSwitch
 */
#define EXPECT_HW_CALL(sw, method) \
  EXPECT_CALL(*getMockHw((sw).get()), method)

/*
 * Convenience macro that wraps EXPECT_CALL() on the underlying MockPlatform
 */
#define EXPECT_PLATFORM_CALL(sw, method) \
  EXPECT_CALL(*getMockPlatform((sw).get()), method)

/*
 * Helper functions for comparing buffer-like objects
 */
#define EXPECT_BUF_EQ(a, b) EXPECT_EQ(fbossHexDump(a), fbossHexDump(b))
#define ASSERT_BUF_EQ(a, b) ASSERT_EQ(fbossHexDump(a), fbossHexDump(b))

std::string fbossHexDump(const folly::IOBuf* buf);
std::string fbossHexDump(const folly::IOBuf& buf);
std::string fbossHexDump(folly::ByteRange buf);
std::string fbossHexDump(folly::StringPiece buf);
std::string fbossHexDump(const std::string& buf);

/*
 * Convenience macro for expecting a packet to be transmitted by the switch
 *
 * The name should be a brief description of the match being performed.
 *
 * The matchFn should be a function that accepts a single "const TxPacket*"
 * argument.  If the packet matches it should return void, and if it doesn't
 * match it should throw an exception describing why the packet did not match.
 *
 * Note that you should not use gtest EXPECT_* macros when checking the
 * packet.  Just because one packet is not a match does not mean that another
 * packet will not arrive and match the filter.  For instance, consider the
 * following code:
 *
 *   EXPECT_PKT(sw, "ARP reply", arpReplyChecker).Times(1);
 *   EXPECT_PKT(sw, "ICMP packet", icmpChecker).Times(1);
 *
 * The code expects to see one ARP reply and one ICMP packet, but it may have
 * to call both filters on a packet to see which one it is.
 */
#define EXPECT_PKT(sw, name, matchFn) \
  EXPECT_HW_CALL(sw, sendPacketSwitched_( \
                 TxPacketMatcher::createMatcher(name, matchFn)));

typedef std::function<void(const TxPacket* pkt)> TxMatchFn;

/*
 * A gmock MatcherInterface for matching TxPacket objects.
 */
class TxPacketMatcher :
  public ::testing::MatcherInterface<std::shared_ptr<TxPacket>> {
 public:
  TxPacketMatcher(folly::StringPiece name, TxMatchFn fn);

  static ::testing::Matcher<std::shared_ptr<TxPacket>> createMatcher(
      folly::StringPiece name,
      TxMatchFn&& fn);

  bool MatchAndExplain(std::shared_ptr<TxPacket> pkt,
                       ::testing::MatchResultListener* l) const override;

  void DescribeTo(std::ostream* os) const override;
  void DescribeNegationTo(std::ostream* os) const override;

 private:
  std::string name_;
  TxMatchFn fn_;
};

}} // facebook::fboss
