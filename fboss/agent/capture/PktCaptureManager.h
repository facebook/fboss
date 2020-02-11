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

#include <folly/Range.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace facebook::fboss {

class PktCapture;
class RxPacket;
class SwSwitch;
class TxPacket;

class PktCaptureManager {
 public:
  explicit PktCaptureManager(SwSwitch* sw);
  virtual ~PktCaptureManager();

  /*
   * Start a new packet capture.
   *
   * This method is safe to call from any thread.
   */
  void startCapture(std::unique_ptr<PktCapture> capture);

  void stopCapture(folly::StringPiece name);
  std::unique_ptr<PktCapture> forgetCapture(folly::StringPiece name);

  void stopAllCaptures();
  void forgetAllCaptures();

  /*
   * packetReceived() is called by the SwSwitch whenever a packet is received.
   *
   * This method is safe to call from any thread.
   */
  void packetReceived(const RxPacket* pkt) {
    // We expect that in the common case there will be no active captures
    // running.  Just do a fast check to handle that case.
    if (!capturesRunning_.load(std::memory_order_acquire)) {
      return;
    }
    packetReceivedImpl(pkt);
  }

  /*
   * packetSent() is called by the SwSwitch whenever a packet is transmitted.
   *
   * This method is safe to call from any thread.
   */
  void packetSent(const TxPacket* pkt) {
    // We expect that in the common case there will be no active captures
    // running.  Just do a fast check to handle that case.
    if (!capturesRunning_.load(std::memory_order_acquire)) {
      return;
    }

    packetSentImpl(pkt);
  }

  const std::string& getCaptureDir() const {
    return captureDir_;
  }

  static void checkCaptureName(folly::StringPiece name);

 private:
  // Forbidden copy constructor and assignment operator
  PktCaptureManager(PktCaptureManager const&) = delete;
  PktCaptureManager& operator=(PktCaptureManager const&) = delete;

  template <typename Fn>
  void invokeCaptures(const Fn& fn);
  void packetReceivedImpl(const RxPacket* pkt);
  void packetSentImpl(const TxPacket* pkt);

  std::atomic<bool> capturesRunning_{false};

  std::mutex mutex_;
  std::string captureDir_;
  std::map<std::string, std::unique_ptr<PktCapture>> activeCaptures_;
  std::map<std::string, std::unique_ptr<PktCapture>> inactiveCaptures_;
};

} // namespace facebook::fboss
