/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/PcapQueue.h"

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/capture/PcapPkt.h"

DEFINE_int32(
    fboss_pcap_queue_depth,
    10240,
    "When taking packet captures, the maximum number of packets "
    "to buffer in memory while waiting them to be written to the "
    "capture file");

namespace facebook::fboss {

PcapQueue::PcapQueue(uint32_t pktCapacity, uint64_t bytesCapacity)
    : pktCapacity_(
          pktCapacity == 0 ? FLAGS_fboss_pcap_queue_depth : pktCapacity),
      bytesCapacity_(bytesCapacity) {
  queue_.reserve(pktCapacity_);
}

PcapQueue::~PcapQueue() {}

template <typename PktType>
void PcapQueue::addPktInternal(const PktType* pkt) {
  // Check to see if this would exceed the queue capacity.
  if (queue_.size() >= pktCapacity_) {
    pktsDropped_ += 1;
    return;
  }
  auto newBytes = bytesInQueue_ + pkt->buf()->computeChainDataLength();
  if (bytesCapacity_ > 0 && newBytes >= bytesCapacity_) {
    pktsDropped_ += 1;
    return;
  }

  queue_.emplace_back(pkt);
}

void PcapQueue::addPkt(const RxPacket* pkt) {
  {
    std::lock_guard<std::mutex> guard(mutex_);
    addPktInternal(pkt);
  }
  cv_.notify_one();
}

void PcapQueue::addPktLocked(const RxPacket* pkt) {
  addPktInternal(pkt);
  // It is preferred not to be holding the lock when we signal cv_,
  // but it is okay to call it with the lock held anyway.  (Having the lock
  // held will just prevent the reader thread from being able to acquire it
  // immediately when the reader wakes.)
  cv_.notify_one();
}

void PcapQueue::addPkt(const TxPacket* pkt) {
  {
    std::lock_guard<std::mutex> guard(mutex_);
    addPktInternal(pkt);
  }
  cv_.notify_one();
}

void PcapQueue::addPktLocked(const TxPacket* pkt) {
  addPktInternal(pkt);
  // It is preferred not to be holding the lock when we signal cv_,
  // but it is okay to call it with the lock held anyway.  (Having the lock
  // held will just prevent the reader thread from being able to acquire it
  // immediately when the reader wakes.)
  cv_.notify_one();
}

void PcapQueue::finish() {
  std::lock_guard<std::mutex> guard(mutex_);
  finished_ = true;
  cv_.notify_all();
}

bool PcapQueue::isFinished() const {
  std::lock_guard<std::mutex> guard(mutex_);
  return finished_;
}

uint64_t PcapQueue::numDropped() const {
  std::lock_guard<std::mutex> guard(mutex_);
  return pktsDropped_;
}

bool PcapQueue::wait(std::vector<PcapPkt>* swapQueue) {
  swapQueue->clear();
  swapQueue->reserve(pktCapacity_);

  std::unique_lock<std::mutex> guard(mutex_);
  while (queue_.empty() && !finished_) {
    cv_.wait(guard);
  }
  if (queue_.empty()) {
    DCHECK(finished_);
    queue_.shrink_to_fit();
    return false;
  }

  swapQueue->swap(queue_);
  bytesInQueue_ = 0;
  return true;
}

} // namespace facebook::fboss
