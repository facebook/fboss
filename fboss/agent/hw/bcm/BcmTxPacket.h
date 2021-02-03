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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmTypes.h"

extern "C" {
#include <bcm/pkt.h>
#include <bcm/types.h>
#ifdef INCLUDE_PKTIO
#include <bcm/pktio.h>
#include <bcm/pktio_defs.h>
#endif
}

namespace facebook::fboss {

class BcmTxPacket;
class BcmSwitch;

struct BcmFreeTxBufUserData {
  BcmFreeTxBufUserData(const BcmPacketT& bcmPacket, const BcmSwitch* bcmSwitch)
      : bcmPacket(bcmPacket), bcmSwitch(bcmSwitch) {}
  const BcmPacketT bcmPacket;
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

  const BcmPacketT& getPkt() {
    return bcmPacket_;
  }

  typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;
  const TimePoint& getQueueTime() const {
    return queued_;
  }

  /*
   * set dest port and set mod to 0
   */
  void setDestModPort(bcm_port_t port) {
    if (!bcmPacket_.usePktIO) {
      DCHECK_EQ((port & ~0xff), 0);
      bcm_pkt_t* pkt = bcmPacket_.ptrUnion.pkt;
      // Need to reset TX_ETHER for packets that are
      // predetermined to go out of a port/trunk
      pkt->flags &= ~BCM_TX_ETHER;
      BCM_PBMP_PORT_SET(pkt->tx_pbmp, port);
      BCM_PBMP_PORT_SET(pkt->tx_upbmp, port);
    } else {
#ifdef INCLUDE_PKTIO
      port_ = port;
#endif
    }
  }

  void setCos(uint8_t cos) noexcept;

#ifdef INCLUDE_PKTIO
  void setSwitched(bool switched) {
    switched_ = switched;
  }
  bool getSwitched(void) {
    return switched_;
  }
#endif

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

  BcmPacketT bcmPacket_;

  // time point when the packet is queued to HW
  TimePoint queued_;

#ifdef INCLUDE_PKTIO
  int unit_{-1};
  // If not switched, directly send to port, bypassing the pipeline
  bool switched_{true};
  uint8_t cos_{0};
  int port_{0};
#endif
};

} // namespace facebook::fboss
