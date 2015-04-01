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

#include "fboss/agent/TxPacket.h"

extern "C" {
#include <opennsl/pkt.h>
#include <opennsl/types.h>
}

namespace facebook { namespace fboss {

class BcmTxPacket : public TxPacket {
 public:
  BcmTxPacket(int unit, uint32_t size);

  opennsl_pkt_t* getPkt() {
    return pkt_;
  }
  const opennsl_pkt_t* getPkt() const {
    return pkt_;
  }

  typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;
  const TimePoint& getQueueTime() const {
    return queued_;
  }

  /*
   * set dest port and set mod to 0
   */
  void setDestModPort(opennsl_port_t port) {
    DCHECK_EQ((port & ~0xff), 0);
    enableHiGigHeader();
    OPENNSL_PBMP_PORT_SET(pkt_->tx_pbmp, port);
    OPENNSL_PBMP_PORT_SET(pkt_->tx_upbmp, port);
  }

  /*
   * Send a BcmTxPacket asynchronously.
   *
   * This is a static function rather than a regular method so that
   * it can accept the packet in a unique_ptr.  This function assumes ownership
   * of the packet, and will automatically delete it when the async send
   * completes.
   *
   * Returns an OpenNSL error code.
   */
  static int sendAsync(std::unique_ptr<BcmTxPacket> pkt) noexcept;


 private:
  // Forbidden copy constructor and assignment operator
  BcmTxPacket(BcmTxPacket const &) = delete;
  BcmTxPacket& operator=(BcmTxPacket const &) = delete;
  void enableHiGigHeader();

  opennsl_pkt_t* pkt_{nullptr};

  // time point when the packet is queued to HW
  TimePoint queued_;
};

}} // facebook::fboss
