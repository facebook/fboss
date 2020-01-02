/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/PktCaptureManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/capture/PktCapture.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>

using folly::StringPiece;
using std::string;
using std::unique_ptr;

namespace facebook::fboss {

PktCaptureManager::PktCaptureManager(SwSwitch* sw) {
  auto persistDir = sw->getPlatform()->getPersistentStateDir();
  captureDir_ = folly::to<string>(persistDir, "/captures");
  utilCreateDir(captureDir_);
}

PktCaptureManager::~PktCaptureManager() {}

void PktCaptureManager::startCapture(unique_ptr<PktCapture> capture) {
  checkCaptureName(capture->name());

  auto path =
      folly::to<std::string>(captureDir_, "/", capture->name(), ".pcap");

  std::lock_guard<std::mutex> g(mutex_);

  const auto& name = capture->name();
  if (activeCaptures_.find(name) != activeCaptures_.end()) {
    throw FbossError("an active capture named \"", name, "\" already exists");
  }

  capture->start(path);
  capturesRunning_.store(true, std::memory_order_release);
  activeCaptures_[name] = std::move(capture);
}

void PktCaptureManager::stopCapture(StringPiece name) {
  std::lock_guard<std::mutex> g(mutex_);

  auto nameStr = name.str();
  auto it = activeCaptures_.find(nameStr);
  if (it == activeCaptures_.end()) {
    throw FbossError("no active capture found with name \"", name, "\"");
  }
  it->second->stop();
  inactiveCaptures_[nameStr] = std::move(it->second);
  activeCaptures_.erase(it);
  capturesRunning_.store(!activeCaptures_.empty(), std::memory_order_release);
}

unique_ptr<PktCapture> PktCaptureManager::forgetCapture(StringPiece name) {
  std::lock_guard<std::mutex> g(mutex_);
  auto nameStr = name.str();
  auto activeIt = activeCaptures_.find(nameStr);
  if (activeIt != activeCaptures_.end()) {
    std::unique_ptr<PktCapture> capture = std::move(activeIt->second);
    activeCaptures_.erase(activeIt);
    capturesRunning_.store(!activeCaptures_.empty(), std::memory_order_release);
    capture->stop();
    return capture;
  }

  auto inactiveIt = inactiveCaptures_.find(nameStr);
  if (inactiveIt != inactiveCaptures_.end()) {
    std::unique_ptr<PktCapture> capture = std::move(inactiveIt->second);
    inactiveCaptures_.erase(inactiveIt);
    return capture;
  }

  throw FbossError("no capture found with name \"", name, "\"");
}

void PktCaptureManager::stopAllCaptures() {
  std::lock_guard<std::mutex> g(mutex_);

  // FIXME
}

void PktCaptureManager::forgetAllCaptures() {
  std::lock_guard<std::mutex> g(mutex_);

  // FIXME
}

template <typename Fn>
void PktCaptureManager::invokeCaptures(const Fn& fn) {
  std::lock_guard<std::mutex> g(mutex_);

  for (auto it = activeCaptures_.begin(); it != activeCaptures_.end();
       /* increment in loop */) {
    // Increment the iterator now, in case this capture deactivates itself and
    // we need to remove it from the active captures.
    auto thisIt = it;
    ++it;
    PktCapture* capture = thisIt->second.get();
    bool stillActive = false;
    try {
      stillActive = fn(capture);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "error when processing packet for capture "
                << capture->name() << " : " << folly::exceptionStr(ex);
      stillActive = false;
    }

    if (!stillActive) {
      XLOG(INFO) << "auto-stopping packet capture \"" << capture->name()
                 << "\"";
      try {
        inactiveCaptures_[capture->name()] = std::move(thisIt->second);
      } catch (const std::exception& ex) {
        XLOG(ERR) << "error adding capture " << capture->name()
                  << " to the inactive list";
        // Can't do much else here.  Just continue and forget the capture.
      }
      activeCaptures_.erase(thisIt);
    }
  }

  bool running = !activeCaptures_.empty();
  capturesRunning_.store(running, std::memory_order_release);
}

void PktCaptureManager::packetReceivedImpl(const RxPacket* pkt) {
  invokeCaptures(
      [&](PktCapture* capture) { return capture->packetReceived(pkt); });
}

void PktCaptureManager::packetSentImpl(const TxPacket* pkt) {
  invokeCaptures([&](PktCapture* capture) { return capture->packetSent(pkt); });
}

void PktCaptureManager::checkCaptureName(folly::StringPiece name) {
  // We use the capture name for the on-disk filename, so don't allow
  // directory separator characters or nul bytes.
  std::string invalidChars("\0/", 2);
  size_t idx = name.find_first_of(invalidChars);
  if (idx != std::string::npos) {
    throw FbossError(
        "invalid capture name \"", name, "\": invalid character at byte ", idx);
  }
}

} // namespace facebook::fboss
