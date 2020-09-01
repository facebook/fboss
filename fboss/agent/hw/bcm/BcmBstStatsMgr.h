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

#include "fboss/agent/hw/BufferStatsLogger.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <atomic>

namespace facebook::fboss {

/*
 * Class for BST configuration and stats update
 */
class BcmBstStatsMgr {
 public:
  BcmBstStatsMgr(BcmSwitch* hw);

  bool isBufferStatCollectionEnabled() const {
    return bufferStatsEnabled_;
  }

  bool startBufferStatCollection();
  bool stopBufferStatCollection();

  bool isFineGrainedBufferStatLoggingEnabled() const {
    return fineGrainedBufferStatsEnabled_;
  }

  bool startFineGrainedBufferStatLogging() {
    if (isBufferStatCollectionEnabled()) {
      fineGrainedBufferStatsEnabled_ = true;
    }
    return fineGrainedBufferStatsEnabled_;
  }

  bool stopFineGrainedBufferStatLogging() {
    fineGrainedBufferStatsEnabled_ = false;
    return !fineGrainedBufferStatsEnabled_;
  }

  uint64_t getDeviceWatermarkBytes() const {
    return deviceWatermarkBytes_.load();
  }

  void updateStats();

 private:
  void syncStats() const;
  void getAndPublishDeviceWatermark();
  void publishDeviceWatermark(uint64_t peakBytes) const;
  void publishQueueuWatermark(
      const std::string& portName,
      int queue,
      uint64_t peakBytes) const;

  BufferStatsLogger* getBufferStatsLogger() const {
    return bufferStatsLogger_.get();
  }

  const BcmSwitch* hw_;
  bool fineGrainedBufferStatsEnabled_{false};
  bool bufferStatsEnabled_{false};
  std::unique_ptr<BufferStatsLogger> bufferStatsLogger_;
  /*
   * Atomic as the stat is updated in stats thread but
   * maybe read from other threads
   */
  std::atomic<uint64_t> deviceWatermarkBytes_{0};
};

} // namespace facebook::fboss
