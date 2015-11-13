/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmRxPacket.h"

extern "C" {
#include <opennsl/rx.h>
}

using folly::IOBuf;

namespace {

void freeRxBuf(void *ptr, void* arg) {
  intptr_t unit = reinterpret_cast<intptr_t>(arg);
  opennsl_rx_free(unit, ptr);
}

}

namespace facebook { namespace fboss {

BcmRxPacket::BcmRxPacket(const opennsl_pkt_t* pkt)
  : unit_(pkt->unit) {
  // The BCM RX code always uses a single buffer.
  // As long as there is just a single buffer, we don't need to allocate
  // a separate array of opennsl_pkt_blk_t objects.
  CHECK_EQ(pkt->blk_count, 1);
  CHECK_EQ(pkt->pkt_data, &pkt->_pkt_data);

  // The BCM RX code always uses a single buffer.
  // Therefore we don't bother checking to see if we need to create a chain of
  // IOBufs rather than just one.
  CHECK_EQ(pkt->blk_count, 1);
  // The packet contains Ethernet FCS (frame check sequence) at the end before
  // interpacket gap, which we are not interested in.
  uint32_t length = std::max(pkt->pkt_len - 4, 0);
  buf_ = IOBuf::takeOwnership(
      pkt->pkt_data->data,             // void* buf
      length,
      freeRxBuf,                       // FreeFunction freeFn
      reinterpret_cast<void*>(unit_)); // void* userData

  // TODO(aeckert): fix sdk bug where the src_port is a signed 8-bit
  // int.  This causes issues when the logical port numbers exceed
  // 127. For now we work around the issue by casting to an 8-bit
  // unsigned port id, but we should fix this in the sdk.
  srcPort_ = PortID(static_cast<uint8_t>(pkt->src_port));
  srcVlan_ = VlanID(pkt->vlan);
  len_ = length;
}

BcmRxPacket::~BcmRxPacket() {
  // Nothing to do.  The IOBuf destructor will call freeRxBuf()
  // to free the packet data
}

}} // facebook::fboss
