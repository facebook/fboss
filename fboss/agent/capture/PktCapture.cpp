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
#include <folly/logging/xlog.h>
#include <sstream>

using folly::StringPiece;

namespace facebook::fboss {

PktCapture::PktCapture(
    folly::StringPiece name,
    uint64_t maxPackets,
    CaptureDirection direction)
    : PktCapture(name, maxPackets, direction, CaptureFilter()) {}

PktCapture::PktCapture(
    folly::StringPiece name,
    uint64_t maxPackets,
    CaptureDirection direction,
    const CaptureFilter& captureFilter)
    : name_(name.str()),
      maxPackets_(maxPackets),
      direction_(direction),
      packetFilter_(captureFilter) {}

void PktCapture::start(StringPiece path) {
  XLOG(INFO) << "starting packet capture " << toString();
  writer_.start(path, true);
}

void PktCapture::stop() {
  writer_.finish();
  XLOG(INFO) << "Stopped packet capture " << toString(true);
}

bool PktCapture::packetReceived(const RxPacket* pkt) {
  std::lock_guard<std::mutex> guard(writer_.mutex());
  if (direction_ != CaptureDirection::CAPTURE_ONLY_TX &&
      true == packetFilter_.passes(pkt)) {
    ++numPacketsReceived_;
    writer_.addPktLocked(pkt);
  }
  return (numPacketsSent_ + numPacketsReceived_) < maxPackets_;
}

bool PktCapture::packetSent(const TxPacket* pkt) {
  std::lock_guard<std::mutex> guard(writer_.mutex());
  if (direction_ != CaptureDirection::CAPTURE_ONLY_RX) {
    ++numPacketsSent_;
    writer_.addPktLocked(pkt);
  }
  return (numPacketsSent_ + numPacketsReceived_) < maxPackets_;
}

std::string PktCapture::toString(bool withStats) const {
  std::stringstream ss;
  ss << "Name:\"" << name_ << "\", maxPackets:" << maxPackets_ << ", Direction:"
     << ((direction_ == CaptureDirection::CAPTURE_TX_RX)
             ? "Tx and Rx"
             : ((direction_ == CaptureDirection::CAPTURE_ONLY_RX) ? "RX only"
                                                                  : "TX only"));
  if (withStats) {
    ss << ", Packet received:" << numPacketsReceived_
       << ", Packet sent:" << numPacketsSent_;
  }
  return ss.str();
}
} // namespace facebook::fboss
