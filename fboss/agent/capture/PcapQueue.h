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

#include <condition_variable>
#include <mutex>
#include <vector>

namespace facebook::fboss {

class RxPacket;
class TxPacket;
class PcapPkt;

/*
 * PcapQueue stores a queue of PcapPkt objects, for transferring packets
 * from an asynchronous capture thread to a blocking thread that will process
 * the packets.  (For instance, writing them to disk using blocking I/O.)
 *
 * There can only be a single reader.
 */
class PcapQueue {
 public:
  explicit PcapQueue(uint32_t pktCapacity, uint64_t bytesCapacity = 0);
  virtual ~PcapQueue();

  uint32_t getPktCapacity() const {
    // pktCapacity_ is const, so no need for locking
    return pktCapacity_;
  }

  /*
   * Get the mutex protecting this PcapQueue.
   *
   * This is exposed to allow callers to also protect their own data
   * with the same mutex if desired.  Callers should call addPktLocked()
   * instead of addPkt() when they are already holding the PcapQueue mutex.
   */
  std::mutex& mutex() const {
    return mutex_;
  }

  void addPkt(const RxPacket* pkt);
  void addPktLocked(const RxPacket* pkt);
  void addPkt(const TxPacket* pkt);
  void addPktLocked(const TxPacket* pkt);

  /*
   * finish() signals that no more packets will be added to the queue.
   *
   * This causes wait() to return false in the reader thread once the packets
   * currently in the queue have been read.
   */
  void finish();
  bool isFinished() const;

  /*
   * Return the number of packets dropped.
   *
   * If the reader is pulling packets off the queue slower than they are being
   * added, packets will be dropped once the queue reaches its maximum
   * capacity.
   */
  uint64_t numDropped() const;

  /*
   * Wait for new packets from the queue.
   *
   * Note: for best performance, the writer should re-use the same vector
   * for multiple wait() calls.  On subsequent calls the queue will already
   * have the desired capacity, and will not need to reallocate memory.
   */
  bool wait(std::vector<PcapPkt>* swapQueue);

 private:
  // Forbidden copy constructor and assignment operator
  PcapQueue(PcapQueue const&) = delete;
  PcapQueue& operator=(PcapQueue const&) = delete;

  template <typename PktType>
  void addPktInternal(const PktType* pkt);

  mutable std::mutex mutex_;
  std::condition_variable cv_;

  bool finished_{false};
  const uint32_t pktCapacity_{0};
  uint64_t bytesCapacity_{0};
  uint64_t bytesInQueue_{0};
  uint64_t pktsDropped_{0};
  std::vector<PcapPkt> queue_;
};

} // namespace facebook::fboss
