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

#include "fboss/agent/capture/PcapFile.h"
#include "fboss/agent/capture/PcapQueue.h"

#include <thread>

namespace facebook::fboss {

/*
 * PcapWriter listes to a PcapQueue and writes the packets it receives
 * to a pcap file.
 *
 * It performs blocking disk I/O, so it performs the writes in its own thread.
 */
class PcapWriter {
 public:
  explicit PcapWriter(uint32_t maxBufferedPkts = 0);
  explicit PcapWriter(
      folly::StringPiece path,
      bool overwriteExisting = false,
      uint32_t maxBufferedPkts = 0);
  virtual ~PcapWriter();

  void start(folly::StringPiece path, bool overwriteExisting = false);

  /*
   * Get the mutex protecting this PcapWriter.
   *
   * This is exposed to allow callers to also protect their own data
   * with the same mutex if desired.  Callers should call addPktLocked()
   * instead of addPkt() when they are already holding the PcapWriter mutex.
   */
  std::mutex& mutex() const {
    return queue_.mutex();
  }

  void addPkt(const RxPacket* pkt) {
    queue_.addPkt(pkt);
  }
  void addPktLocked(const RxPacket* pkt) {
    queue_.addPktLocked(pkt);
  }
  void addPkt(const TxPacket* pkt) {
    queue_.addPkt(pkt);
  }
  void addPktLocked(const TxPacket* pkt) {
    queue_.addPktLocked(pkt);
  }
  void finish();

  /*
   * Return the number of packets dropped.
   *
   * Packets will be dropped if the writer thread cannot write packets
   * to disk as fast as they are being added.
   */
  uint64_t numDropped() const {
    return queue_.numDropped();
  }

 private:
  // Forbidden copy constructor and assignment operator
  PcapWriter(PcapWriter const&) = delete;
  PcapWriter& operator=(PcapWriter const&) = delete;

  void threadMain();
  void writeHeader();
  void writeLoop();

  PcapFile file_;
  PcapQueue queue_;
  std::exception_ptr ex_;
  std::thread thread_;
};

} // namespace facebook::fboss
