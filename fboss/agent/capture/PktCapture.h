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

#include "fboss/agent/capture/PcapWriter.h"

#include <folly/Range.h>
#include <string>

namespace facebook { namespace fboss {

class RxPacket;
class TxPacket;

/*
 * A packet capture job.
 */
class PktCapture {
 public:
  PktCapture(folly::StringPiece name, uint64_t maxPackets);

  const std::string& name() const {
    return name_;
  }

  void start(folly::StringPiece path);
  void stop();

  bool packetReceived(const RxPacket* pkt);
  bool packetSent(const TxPacket* pkt);

 private:
  // Forbidden copy constructor and assignment operator
  PktCapture(PktCapture const &) = delete;
  PktCapture& operator=(PktCapture const &) = delete;

  const std::string name_;

  // Note: the rest of the state in this class is protcted by
  // the PcapWriter's mutex.
  PcapWriter writer_;
  uint64_t maxPackets_{0};
  uint64_t numPacketsReceived_{0};
};

}} // facebook::fboss
