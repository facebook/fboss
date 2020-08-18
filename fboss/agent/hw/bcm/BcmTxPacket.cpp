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

#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/tx.h>
}

using folly::IOBuf;
using std::unique_ptr;

namespace {

using namespace facebook::fboss;
void freeTxBuf(void* /*ptr*/, void* arg) {
  // Put the BcmTxIoBufUserData back into a unique_ptr.
  // This will delete it when we return.
  unique_ptr<facebook::fboss::BcmFreeTxBufUserData> freeTxBufUserData(
      static_cast<facebook::fboss::BcmFreeTxBufUserData*>(arg));
  bcm_pkt_t* pkt = freeTxBufUserData->pkt;
  int rv = bcm_pkt_free(pkt->unit, pkt);
  bcmLogError(rv, "Failed to free packet");
  freeTxBufUserData->bcmSwitch->getSwitchStats()->txPktFree();
}

inline void txCallbackImpl(int /*unit*/, bcm_pkt_t* pkt, void* cookie) {
  // Put the BcmTxCallbackUserData back into a unique_ptr.
  // This will delete it when we return.
  unique_ptr<facebook::fboss::BcmTxCallbackUserData> bcmTxCbUserData(
      static_cast<facebook::fboss::BcmTxCallbackUserData*>(cookie));
  DCHECK_EQ(pkt, bcmTxCbUserData->txPacket->getPkt());

  // Now we reset the pkt buffer back to what was originally allocated
  pkt->pkt_data->data = bcmTxCbUserData->txPacket->buf()->writableBuffer();

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end - bcmTxCbUserData->txPacket->getQueueTime());
  bcmTxCbUserData->bcmSwitch->getSwitchStats()->txSentDone(duration.count());
}
} // namespace

namespace facebook::fboss {

std::mutex& BcmTxPacket::syncPktMutex() {
  static std::mutex _syncPktMutex;
  return _syncPktMutex;
}

std::condition_variable& BcmTxPacket::syncPktCV() {
  static std::condition_variable _syncPktCV;
  return _syncPktCV;
}

bool& BcmTxPacket::syncPacketSent() {
  static bool _syncPktSent{false};
  return _syncPktSent;
}

BcmTxPacket::BcmTxPacket(int unit, uint32_t size, const BcmSwitch* bcmSwitch)
    : queued_(std::chrono::time_point<std::chrono::steady_clock>::min()) {
  int rv = bcm_pkt_alloc(unit, size, BCM_TX_CRC_APPEND | BCM_TX_ETHER, &pkt_);
  if (BCM_FAILURE(rv)) {
    bcmSwitch->getSwitchStats()->txPktAllocErrors();
    bcmCheckError(rv, "Failed to allocate packet.");
  } else {
    auto freeTxBufUserData =
        std::make_unique<BcmFreeTxBufUserData>(pkt_, bcmSwitch);
    buf_ = IOBuf::takeOwnership(
        pkt_->pkt_data->data,
        size,
        freeTxBuf,
        reinterpret_cast<void*>(freeTxBufUserData.get()));
    bcmSwitch->getSwitchStats()->txPktAlloc();
    /*
     * Release the unique pointer without destroying the free buffer user
     * data. The unique pointer will be reconstructed when the IOBuf is
     * freed to free the packet buffer as well as to increment the
     * switch stats
     */
    freeTxBufUserData.release();
  }
}

inline int BcmTxPacket::sendImpl(
    unique_ptr<BcmTxPacket> pkt,
    const BcmSwitch* bcmSwitch) noexcept {
  bcm_pkt_t* bcmPkt = pkt->pkt_;
  const auto buf = pkt->buf();

  // TODO(aeckert): Setting the pkt len manually should be replaced in future
  // releases of bcm with BCM_PKT_TX_LEN_SET or bcm_flags_len_setup
  DCHECK(bcmPkt->pkt_data);
  bcmPkt->pkt_data->len = buf->length();

  // Now we also set the buffer that will be sent out to point at
  // buf->writableBuffer in case there is unused header space in the IOBuf
  bcmPkt->pkt_data->data = buf->writableData();
  pkt->queued_ = std::chrono::steady_clock::now();
  auto txCbUserData =
      std::make_unique<BcmTxCallbackUserData>(std::move(pkt), bcmSwitch);
  auto rv = bcm_tx(bcmPkt->unit, bcmPkt, txCbUserData.get());
  if (BCM_SUCCESS(rv)) {
    /*
     * Release the unique pointer without destroying the TxPacket object.
     * The unique pointer will be reconstructed during the Tx callback to
     * reset the packet and to increment the switch stats.
     */
    txCbUserData.release();
    bcmSwitch->getSwitchStats()->txSent();
  } else {
    bcmLogError(rv, "failed to send packet");
    if (rv == BCM_E_MEMORY) {
      bcmSwitch->getSwitchStats()->txPktAllocErrors();
    } else if (rv) {
      bcmSwitch->getSwitchStats()->txError();
    }
  }
  return rv;
}

void BcmTxPacket::txCallbackAsync(int unit, bcm_pkt_t* pkt, void* cookie) {
  txCallbackImpl(unit, pkt, cookie);
}

void BcmTxPacket::txCallbackSync(int unit, bcm_pkt_t* pkt, void* cookie) {
  txCallbackImpl(unit, pkt, cookie);
  std::lock_guard<std::mutex> lk(syncPktMutex());
  syncPacketSent() = true;
  syncPktCV().notify_one();
}

int BcmTxPacket::sendAsync(
    unique_ptr<BcmTxPacket> pkt,
    const BcmSwitch* bcmSwitch) noexcept {
  bcm_pkt_t* bcmPkt = pkt->pkt_;
  DCHECK(bcmPkt->call_back == nullptr);
  bcmPkt->call_back = BcmTxPacket::txCallbackAsync;
  return sendImpl(std::move(pkt), bcmSwitch);
}

int BcmTxPacket::sendSync(
    unique_ptr<BcmTxPacket> pkt,
    const BcmSwitch* bcmSwitch) noexcept {
  bcm_pkt_t* bcmPkt = pkt->pkt_;
  DCHECK(bcmPkt->call_back == nullptr);
  bcmPkt->call_back = BcmTxPacket::txCallbackSync;
  {
    std::lock_guard<std::mutex> lk{syncPktMutex()};
    syncPacketSent() = false;
  }
  // Give up the lock when calling sendPacketImpl. Callback
  // on packet send complete, maybe invoked immediately on
  // the same thread. Since the callback needs to acquire
  // the same lock (to mark send as done). We need to give
  // up the lock here to avoid deadlock. Further since
  // sendImpl does not update any shared data structures
  // its optimal to give up the lock anyways.
  auto rv = sendImpl(std::move(pkt), bcmSwitch);
  std::unique_lock<std::mutex> lk{syncPktMutex()};
  syncPktCV().wait(lk, [] { return syncPacketSent(); });
  return rv;
}

void BcmTxPacket::setCos(uint8_t cos) {
  pkt_->cos = cos;
}

} // namespace facebook::fboss
