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

#include "fboss/agent/RxPacket.h"

#include <vector>

extern "C" {
#include <bcm/pkt.h>
}

namespace facebook::fboss {

class BcmRxPacket : public RxPacket {
 public:
  /*
   * Create a BcmRxPacket from a bcm_pkt_t received by an rx callback.
   *
   * The rx callback should return BCM_RX_HANDLED_OWNED after successful
   * creation of the BcmRxPacket.
   */
  explicit BcmRxPacket(const bcm_pkt_t* pkt);

  ~BcmRxPacket() override;

 private:
  int unit_{-1};
};

// TODO (skhare) Simplify/fold into BcmRxPacket
class BcmSwitch;

class FbBcmRxPacket : public BcmRxPacket {
 public:
  explicit FbBcmRxPacket(const bcm_pkt_t* pkt, const BcmSwitch* bcmSwitch);

  std::string describeDetails() const override;
  int cosQueue() const override {
    return _cosQueue;
  }
  std::vector<RxReason> getReasons() override;

 private:
  // Forbidden copy constructor and assignment operator
  FbBcmRxPacket(FbBcmRxPacket const&) = delete;
  FbBcmRxPacket& operator=(FbBcmRxPacket const&) = delete;

  uint8_t _cosQueue{0};
  uint8_t _priority{0};
  bcm_rx_reasons_t _reasons;
};

} // namespace facebook::fboss
