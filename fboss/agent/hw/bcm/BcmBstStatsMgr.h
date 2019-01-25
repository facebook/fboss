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

namespace facebook {
namespace fboss {

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

  void updateStats();

 private:
  void syncStats();
  void exportDeviceBufferUsage();

  BufferStatsLogger* getBufferStatsLogger() const {
    return bufferStatsLogger_.get();
  }

  const BcmSwitch* hw_;
  bool fineGrainedBufferStatsEnabled_{false};
  bool bufferStatsEnabled_{false};
  std::unique_ptr<BufferStatsLogger> bufferStatsLogger_;
};
} // namespace fboss
} // namespace facebook
