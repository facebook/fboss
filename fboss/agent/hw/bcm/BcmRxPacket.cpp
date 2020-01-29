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

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/RxUtils.h"

#include <folly/Format.h>
#include <tuple>

extern "C" {
#include <bcm/rx.h>
}

using folly::IOBuf;

namespace {

void freeRxBuf(void* ptr, void* arg) {
  intptr_t unit = reinterpret_cast<intptr_t>(arg);
  bcm_rx_free(unit, ptr);
}

const char* const kRxReasonNames[] = BCM_RX_REASON_NAMES_INITIALIZER;

} // namespace

namespace facebook::fboss {

BcmRxPacket::BcmRxPacket(const bcm_pkt_t* pkt) : unit_(pkt->unit) {
  // The BCM RX code always uses a single buffer.
  // As long as there is just a single buffer, we don't need to allocate
  // a separate array of bcm_pkt_blk_t objects.

  CHECK_EQ(pkt->blk_count, 1);
  CHECK_EQ(pkt->pkt_data, &pkt->_pkt_data);

  buf_ = IOBuf::takeOwnership(
      pkt->pkt_data->data, // void* buf
      pkt->pkt_len,
      freeRxBuf, // FreeFunction freeFn
      reinterpret_cast<void*>(unit_)); // void* userData

  srcPort_ = PortID(pkt->src_port);
  srcVlan_ = VlanID(pkt->vlan);
  len_ = pkt->pkt_len;
}

BcmRxPacket::~BcmRxPacket() {
  // Nothing to do.  The IOBuf destructor will call freeRxBuf()
  // to free the packet data
}

FbBcmRxPacket::FbBcmRxPacket(const bcm_pkt_t* pkt, const BcmSwitch* bcmSwitch)
    : BcmRxPacket(pkt), _cosQueue(pkt->cos) {
  auto bcm_pkt = reinterpret_cast<const bcm_pkt_t*>(pkt);

  if (pkt->flags & BCM_PKT_F_TRUNK) {
    isFromAggregatePort_ = true;
    srcAggregatePort_ =
        bcmSwitch->getTrunkTable()->getAggregatePortId(bcm_pkt->src_trunk);
  }

  _priority = bcm_pkt->prio_int;
  _reasons = bcm_pkt->rx_reasons;
}

std::string FbBcmRxPacket::describeDetails() const {
  return folly::sformat(
      "cos={} priority={} reasons={}",
      _cosQueue,
      _priority,
      RxUtils::describeReasons(_reasons));
}

std::vector<BcmRxPacket::RxReason> FbBcmRxPacket::getReasons() {
  std::vector<BcmRxPacket::RxReason> reasons;
  for (int n = 0; n < bcmRxReasonCount; n++) {
    if (BCM_RX_REASON_GET(_reasons, n)) {
      reasons.push_back({n, kRxReasonNames[n]});
    }
  }
  return reasons;
}

} // namespace facebook::fboss
