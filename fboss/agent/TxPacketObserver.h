// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#pragma once
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss {

class TxPacketObserver {
 public:
  /*
   * This function is called after the thrift server sends the packet to the hw
   * agent.
   */
  void afterFreeExtBuffer() const noexcept {
    TxPacket::decrementPacketCounter();
  }
  /*
   * This function is called after buffer ownership changes. In this case, when
   * the thrift server takes over ownership of the buffer, this function is
   * called. For our observer, we DO NOT want to increment packet counter when
   * ownership changes.
   */
  void afterReleaseExtBuffer() const noexcept {}
};

} // namespace facebook::fboss
