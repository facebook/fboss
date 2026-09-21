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
#include "fboss/agent/hw/bcm/BcmTypes.h"

#include <vector>

extern "C" {
#include <bcm/pkt.h>
#ifdef INCLUDE_PKTIO
#include <bcm/pktio.h>
#include <bcm/pktio_defs.h>
#endif
}

namespace facebook::fboss {

struct BcmRxReasonsT {
  bool usePktIO;
  union {
    bcm_rx_reasons_t reasons;
#ifdef INCLUDE_PKTIO
    bcm_pktio_reasons_t pktio_reasons;
#endif
  } reasonsUnion;
};

class BcmRxPacket : public RxPacket {
 public:
  /*
   * Create a BcmRxPacket from a bcm_pkt_t received by an rx callback.
   *
   * The rx callback should return BCM_RX_HANDLED_OWNED after successful
   * creation of the BcmRxPacket.
   */
  explicit BcmRxPacket(const BcmPacketT& bcmPktPtr);

  ~BcmRxPacket() override;

 protected:
  int unit_{-1};
  bool usePktIO_{false};
};

// TODO (skhare) Simplify/fold into BcmRxPacket
class BcmSwitch;

class FbBcmRxPacket : public BcmRxPacket {
 public:
  explicit FbBcmRxPacket(
      const BcmPacketT& bcmPktPtr,
      const BcmSwitch* bcmSwitch);

  std::string describeDetails() const override;
  std::vector<RxReason> getReasons() override;

 private:
  // Forbidden copy constructor and assignment operator
  FbBcmRxPacket(FbBcmRxPacket const&) = delete;
  FbBcmRxPacket& operator=(FbBcmRxPacket const&) = delete;

  uint8_t _priority{0};
  BcmRxReasonsT _reasons;
};

} // namespace facebook::fboss
