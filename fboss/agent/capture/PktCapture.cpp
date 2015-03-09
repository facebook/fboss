/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/PktCapture.h"

#include <folly/Conv.h>

using folly::StringPiece;

namespace facebook { namespace fboss {

PktCapture::PktCapture(folly::StringPiece name, uint64_t maxPackets)
  : name_(name.str()),
    maxPackets_(maxPackets) {
}

void PktCapture::start(StringPiece path) {
  writer_.start(path, true);
}

void PktCapture::stop() {
  writer_.finish();
}

bool PktCapture::packetReceived(const RxPacket* pkt) {
  std::lock_guard<std::mutex> guard(writer_.mutex());
  ++numPacketsReceived_;
  writer_.addPktLocked(pkt);
  return numPacketsReceived_ < maxPackets_;
}

bool PktCapture::packetSent(const TxPacket* pkt) {
  std::lock_guard<std::mutex> guard(writer_.mutex());
  ++numPacketsReceived_;
  writer_.addPktLocked(pkt);
  return numPacketsReceived_ < maxPackets_;
}

}} // facebook::fboss
