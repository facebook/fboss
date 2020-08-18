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

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "fboss/agent/TxPacket.h"

extern "C" {
#include <bcm/pkt.h>
#include <bcm/types.h>
}

namespace facebook::fboss {

class BcmTxPacket;
class BcmSwitch;

struct BcmFreeTxBufUserData {
  BcmFreeTxBufUserData(bcm_pkt_t* pkt, const BcmSwitch* bcmSwitch)
      : pkt(pkt), bcmSwitch(bcmSwitch) {}
  bcm_pkt_t* pkt;
  const BcmSwitch* bcmSwitch;
};

struct BcmTxCallbackUserData {
  BcmTxCallbackUserData(
      std::unique_ptr<BcmTxPacket> txPacket,
      const BcmSwitch* bcmSwitch)
      : txPacket(std::move(txPacket)), bcmSwitch(bcmSwitch) {}
  std::unique_ptr<BcmTxPacket> txPacket;
  const BcmSwitch* bcmSwitch;
};

class BcmTxPacket : public TxPacket {
 public:
  BcmTxPacket(int unit, uint32_t size, const BcmSwitch* bcmSwitch);

  bcm_pkt_t* getPkt() {
    return pkt_;
  }
  const bcm_pkt_t* getPkt() const {
    return pkt_;
  }

  typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;
  const TimePoint& getQueueTime() const {
    return queued_;
  }

  /*
   * set dest port and set mod to 0
   */
  void setDestModPort(bcm_port_t port) {
    DCHECK_EQ((port & ~0xff), 0);
    // Need to reset TX_ETHER for packets that are
    // predetermined to go out of a port/trunk
    pkt_->flags &= ~BCM_TX_ETHER;
    BCM_PBMP_PORT_SET(pkt_->tx_pbmp, port);
    BCM_PBMP_PORT_SET(pkt_->tx_upbmp, port);
  }

  void setCos(uint8_t cos);

  /*
   * Send a BcmTxPacket asynchronously.
   *
   * This is a static function rather than a regular method so that
   * it can accept the packet in a unique_ptr.  This function assumes ownership
   * of the packet, and will automatically delete it when the async send
   * completes.
   *
   * Returns an Bcm error code.
   */
  static int sendAsync(
      std::unique_ptr<BcmTxPacket> pkt,
      const BcmSwitch* bcmSwitch) noexcept;
  /*
   * Send a BcmTxPacket synchronously.
   *
   * This is a static function rather than a regular method so that
   * it can accept the packet in a unique_ptr.  This function assumes ownership
   * of the packet, and will automatically delete it when the sync send
   * completes.
   *
   * Returns an Bcm error code.
   */
  static int sendSync(
      std::unique_ptr<BcmTxPacket> pkt,
      const BcmSwitch* bcmSwitch) noexcept;

 private:
  inline static int sendImpl(
      std::unique_ptr<BcmTxPacket> pkt,
      const BcmSwitch* bcmSwitch) noexcept;
  static void txCallbackAsync(int unit, bcm_pkt_t* pkt, void* cookie);
  static void txCallbackSync(int unit, bcm_pkt_t* pkt, void* cookie);

  // Forbidden copy constructor and assignment operator
  BcmTxPacket(BcmTxPacket const&) = delete;
  BcmTxPacket& operator=(BcmTxPacket const&) = delete;

  // Synchronization around synchrnous packet sending
  static std::mutex& syncPktMutex();
  static std::condition_variable& syncPktCV();
  static bool& syncPacketSent();

  bcm_pkt_t* pkt_{nullptr};

  // time point when the packet is queued to HW
  TimePoint queued_;
};

} // namespace facebook::fboss
