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

#include <fmt/core.h>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

namespace {

constexpr auto kMplsTtlExceededCounter = "mpls.ttl_exceeded.sum";
std::unique_ptr<facebook::fboss::HwTestHandle> setupTestHandle() {
  auto config = facebook::fboss::testConfigA();
  auto handle = facebook::fboss::createTestHandle(&config);
  handle->getSw()->initialConfigApplied(std::chrono::steady_clock::now());
  return handle;
}

folly::IOBuf makeMplsPacket(uint8_t ttl) {
  const auto ttlHex = fmt::format("{:02x}", ttl);
  return facebook::fboss::PktUtil::parseHexData(
      // Destination and source MAC.
      "00 02 00 00 00 01"
      "00 02 00 00 00 02"
      // MPLS unicast EtherType.
      "88 47"
      // Label 100, traffic class 0, bottom of stack 1, variable TTL.
      "00 06 41" +
      ttlHex +
      // IPv4 payload. TTL expiry is determined from the MPLS top-label TTL,
      // so payload bytes are irrelevant for this counter.
      "45 00 00 14 00 00 00 00 40 11 00 00 0a 00 00 01 0a 00 00 02");
}

void rxMplsPacket(
    facebook::fboss::HwTestHandle* handle,
    uint8_t ttl,
    facebook::fboss::PortID port,
    facebook::fboss::VlanID vlan) {
  auto pkt = makeMplsPacket(ttl);
  handle->rxPacket(
      std::make_unique<folly::IOBuf>(pkt),
      facebook::fboss::PortDescriptor(port),
      vlan);
}

} // namespace

namespace facebook::fboss {

TEST(MPLSHandlerTest, TtlExpiredPacketIncrementsCounter) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  CounterCache counters(sw);

  rxMplsPacket(handle.get(), 1 /* ttl */, PortID(1), VlanID(1));

  counters.update();
  // TTL is 1, so the TTL exceeded counter is incremented.
  counters.checkDelta(
      SwitchStats::kCounterPrefix + kMplsTtlExceededCounter,
      1 /* expectedDelta */);
}

TEST(MPLSHandlerTest, NonExpiredTtlDoesNotIncrementCounter) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  CounterCache counters(sw);

  rxMplsPacket(handle.get(), 2 /* ttl */, PortID(1), VlanID(1));

  counters.update();
  // TTL is greater than 1, so the TTL exceeded counter is not incremented.
  counters.checkDelta(
      SwitchStats::kCounterPrefix + kMplsTtlExceededCounter,
      0 /* expectedDelta */);
}

} // namespace facebook::fboss
