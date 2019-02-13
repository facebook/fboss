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

#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <folly/Optional.h>

namespace facebook { namespace fboss {

class MockPlatform;

/*
 * MockableHwSwitch is a mockable implementation of a HwSwitch. This can
 * be run in two modes:
 *
 * 1) As a pure mock.
 * In this mode, non-critical functions have stub implemntations and functions
 * that we need to control for tests are mocked with gmock.
 *
 * 2) By delegating to a real HwSwitch implementation.
 * In this mode, all functions invoke the corresponding method on the
 * actual HwSwitch. This is true for mocked gmock functions as
 * well. Those functions will invoke the same function on the real
 * hwswitch by default. Note that it is still possible to force these
 * functions to return a specific value or check if they have been
 * called using gmock. This makes it easy to convert our existing
 * tests to work against a real HwSwitch and add more introspection in
 * to what we expect the HwSwitch to be doing.
 *
 * If a non-null value is passed in as 'realHw' in the constructor, we
 * use mode 2. Otherwise, we use mode 1.
 */
class MockableHwSwitch : public MockHwSwitch {
 public:
  MockableHwSwitch(MockPlatform* platform, HwSwitch* realHw);

  std::unique_ptr<TxPacket> allocatePacket(uint32_t) override;

  /*
   * These 'Adaptor' methods are super hacky, but are needed because
   * gmock does not support mocking move-only types. Note that this may
   * change in gmock (https://github.com/google/googletest/issues/395).
   * If those patches land we should remove this hack. Since we cannot
   * mock unique_ptr, we need to convert the mocked call to use a raw
   * ptr. However, in the case of using a real hwswitch, we need to
   * convert back to a unique_ptr so we match the real interface (and
   * the packet memory is safely owned).
   */
  bool sendPacketSwitchedAsyncAdaptor(TxPacket* pkt) noexcept;
  bool sendPacketOutOfPortAsyncAdaptor(
      TxPacket* pkt,
      PortID port,
      folly::Optional<uint8_t> cos = folly::none) noexcept;
  bool sendPacketSwitchedSyncAdaptor(TxPacket* pkt) noexcept;
  bool sendPacketOutOfPortSyncAdaptor(TxPacket* pkt, PortID port) noexcept;

 private:
  // Forbidden copy constructor and assignment operator
  MockableHwSwitch(MockableHwSwitch const &) = delete;
  MockableHwSwitch& operator=(MockableHwSwitch const &) = delete;

  HwSwitch* realHw_{nullptr};
};

}} // facebook::fboss
