/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTxPacket.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmStats.h"

extern "C" {
#include <opennsl/tx.h>
}

using folly::IOBuf;
using std::unique_ptr;

namespace {

using namespace facebook::fboss;

void freeTxBuf(void* /*ptr*/, void* arg) {
  opennsl_pkt_t* pkt = reinterpret_cast<opennsl_pkt_t*>(arg);
  int rv = opennsl_pkt_free(pkt->unit, pkt);
  bcmLogError(rv, "Failed to free packet");
  BcmStats::get()->txPktFree();
}

void txCallback(int /*unit*/, opennsl_pkt_t* pkt, void* cookie) {
  // Put the BcmTxPacket back into a unique_ptr.
  // This will delete it when we return.
  unique_ptr<facebook::fboss::BcmTxPacket> bcmTxPkt(
      static_cast<facebook::fboss::BcmTxPacket*>(cookie));
  DCHECK_EQ(pkt, bcmTxPkt->getPkt());

  // Now we reset the pkt buffer back to what was originally allocated
  pkt->pkt_data->data = bcmTxPkt->buf()->writableBuffer();

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end - bcmTxPkt->getQueueTime());
  BcmStats::get()->txSentDone(duration.count());
}

}

namespace facebook { namespace fboss {

BcmTxPacket::BcmTxPacket(int unit, uint32_t size)
    : queued_(std::chrono::time_point<std::chrono::steady_clock>::min()) {
  int rv = opennsl_pkt_alloc(unit, size,
                             OPENNSL_TX_CRC_APPEND | OPENNSL_TX_ETHER, &pkt_);
  bcmCheckError(rv, "Failed to allocate packet.");
  buf_ = IOBuf::takeOwnership(pkt_->pkt_data->data, size,
                              freeTxBuf, reinterpret_cast<void*>(pkt_));
  BcmStats::get()->txPktAlloc();
}

void BcmTxPacket::enableHiGigHeader() {
  // this is a hack, as ideally, we just want to reset TX_ETHER flag, but
  // opennsl does not have api to read flags and set or reset bits
  // incrementally.
  pkt_->flags &= ~OPENNSL_TX_ETHER;
}

int BcmTxPacket::sendAsync(unique_ptr<BcmTxPacket> pkt) noexcept {
  opennsl_pkt_t* bcmPkt = pkt->pkt_;
  DCHECK(bcmPkt->call_back == nullptr);
  bcmPkt->call_back = txCallback;
  const auto buf = pkt->buf();

  // TODO(aeckert): Setting the pkt len manually should be replaced in future
  // releases of opennsl with OPENNSL_PKT_TX_LEN_SET or opennsl_flags_len_setup
  DCHECK(bcmPkt->pkt_data);
  bcmPkt->pkt_data->len = buf->length();

  // Now we also set the buffer that will be sent out to point at
  // buf->writableBuffer in case there is unused header space in the IOBuf
  bcmPkt->pkt_data->data = buf->writableData();

  pkt->queued_ = std::chrono::steady_clock::now();
  auto rv = opennsl_tx(bcmPkt->unit, bcmPkt, pkt.get());
  if (OPENNSL_SUCCESS(rv)) {
    pkt.release();
    BcmStats::get()->txSent();
  } else {
    bcmLogError(rv, "failed to send packet");
    if (rv == OPENNSL_E_MEMORY) {
      BcmStats::get()->txPktAllocErrors();
    } else if (rv) {
      BcmStats::get()->txError();
    }
  }
  return rv;
}

}} // facebook::fboss
