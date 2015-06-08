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

extern "C" {
#include <opennsl/pkt.h>
}

namespace facebook { namespace fboss {

class BcmRxPacket : public RxPacket {
 public:
  /*
   * Create a BcmRxPacket from a opennsl_pkt_t received by an rx callback.
   *
   * The rx callback should return OPENNSL_RX_HANDLED_OWNED after successful
   * creation of the BcmRxPacket.
   */
  explicit BcmRxPacket(const opennsl_pkt_t* pkt);

  ~BcmRxPacket() override;

 private:
  int unit_{-1};
};

}} // facebook::fboss
