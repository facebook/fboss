/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/logging/xlog.h>

#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

bool BcmBstStatsMgr::startBufferStatCollection() {
  if (!isBufferStatCollectionEnabled()) {
    hw_->getCosMgr()->enableBst();
    bufferStatsEnabled_ = true;
  }

  return bufferStatsEnabled_;
}

bool BcmBstStatsMgr::stopBufferStatCollection() {
  if (isBufferStatCollectionEnabled()) {
    hw_->getCosMgr()->disableBst();
    bufferStatsEnabled_ = false;
  }
  return !bufferStatsEnabled_;
}

void BcmBstStatsMgr::syncStats() const {
  auto rv = bcm_cosq_bst_stat_sync(
      hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdUcast);
  bcmCheckError(rv, "Failed to sync BST stat");
}

void BcmBstStatsMgr::getAndPublishDeviceWatermark() {
  auto peakUsage = hw_->getCosMgr()->deviceStatGet(bcmBstStatIdDevice);
  auto peakBytes = peakUsage * hw_->getMMUCellBytes();
  if (peakBytes > hw_->getMMUBufferBytes()) {
    // First value read immediately after enabling BST, gives
    // a abnormally high usage value (over 1B bytes), which is
    // obviously bogus. Warn, but skip writing that value.
    XLOG(WARNING)
        << " Got a bogus switch MMU buffer utilization value, peak cells: "
        << peakUsage << " peak bytes: " << peakBytes << " skipping write";
    return;
  }

  deviceWatermarkBytes_.store(peakBytes);
  publishDeviceWatermark(peakBytes);
  if (isFineGrainedBufferStatLoggingEnabled()) {
    bufferStatsLogger_->logDeviceBufferStat(
        peakBytes, hw_->getMMUBufferBytes());
  }
}

void BcmBstStatsMgr::updateStats() {
  syncStats();

  for (const auto& entry : *hw_->getPortTable()) {
    BcmPort* bcmPort = entry.second;

    auto curPortStatsOptional = bcmPort->getPortStats();
    if (!curPortStatsOptional) {
      // ignore ports with no saved stats
      continue;
    }

    auto cosMgr = hw_->getCosMgr();

    std::map<int16_t, int64_t> queueId2WatermarkBytes;
    auto qosSupported =
        hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS);
    auto maxQueueId = qosSupported
        ? bcmPort->getQueueManager()->getNumQueues(cfg::StreamType::UNICAST) - 1
        : 0;
    for (int queue = 0; queue <= maxQueueId; queue++) {
      auto peakBytes = 0;

      if (bcmPort->isUp()) {
        auto peakCells = cosMgr->statGet(
            PortID(bcmPort->getBcmPortId()), queue, bcmBstStatIdUcast);
        peakBytes = peakCells * hw_->getMMUCellBytes();
      }
      queueId2WatermarkBytes[queue] = peakBytes;
      publishQueueuWatermark(bcmPort->getPortName(), queue, peakBytes);

      if (isFineGrainedBufferStatLoggingEnabled()) {
        getBufferStatsLogger()->logPortBufferStat(
            bcmPort->getPortName(),
            BufferStatsLogger::Direction::Egress,
            queue,
            peakBytes,
            curPortStatsOptional->queueOutDiscardBytes__ref()->at(queue),
            bcmPort->getPlatformPort()->getEgressXPEs());
      }
    }
    bcmPort->setQueueWaterMarks(std::move(queueId2WatermarkBytes));
  }
  getAndPublishDeviceWatermark();
}

} // namespace facebook::fboss
