/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"

#include <algorithm>

#include <folly/logging/xlog.h>

#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/if/gen-cpp2/highfreq_types.h"

extern "C" {}

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
  bcmCheckError(rv, "Failed to sync bcmBstStatIdUcast stat");
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    // All the below are PG related and available on platforms supporting PFC
    rv = bcm_cosq_bst_stat_sync(
        hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdPriGroupHeadroom);
    bcmCheckError(rv, "Failed to sync bcmBstStatIdPriGroupHeadroom stat");
    rv = bcm_cosq_bst_stat_sync(
        hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdPriGroupShared);
    bcmCheckError(rv, "Failed to sync bcmBstStatIdPriGroupShared stat");
    rv = bcm_cosq_bst_stat_sync(
        hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdHeadroomPool);
    bcmCheckError(rv, "Failed to sync bcmBstStatIdHeadroomPool stat");
    rv = bcm_cosq_bst_stat_sync(
        hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdIngPool);
    bcmCheckError(rv, "Failed to sync bcmBstStatIdIngPool stat");
  }
}

void BcmBstStatsMgr::syncHighFrequencyStats(
    const HfStatsConfig& statsConfig) const {
  if (statsConfig.includeDeviceWatermark().value()) {
    int rv = bcm_cosq_bst_stat_sync(
        hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdUcast);
    bcmCheckError(rv, "Failed to sync bcmBstStatIdUcast stat");
  }
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  // All the below are PG related and available on platforms supporting PFC
  bool syncPg{false};
  bool syncQueue{false};
  switch (statsConfig.portStatsConfig()->getType()) {
    case HfPortStatsConfig::Type::allPortsConfig: {
      syncPg = statsConfig.portStatsConfig()
                   ->allPortsConfig()
                   ->includePgWatermark()
                   .value();
      syncQueue = statsConfig.portStatsConfig()
                      ->allPortsConfig()
                      ->includeQueueWatermark()
                      .value();
      break;
    }
    case HfPortStatsConfig::Type::filterConfig: {
      syncPg = std::ranges::any_of(
          statsConfig.portStatsConfig()->filterConfig().value(),
          [](const auto& portConfig) {
            return portConfig.second.includePgWatermark().value();
          });
      syncQueue = std::ranges::any_of(
          statsConfig.portStatsConfig()->filterConfig().value(),
          [](const auto& portConfig) {
            return portConfig.second.includeQueueWatermark().value();
          });
      break;
    }
    default:
      break;
  }
  if (syncPg) {
    int rv = bcm_cosq_bst_stat_sync(
        hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdPriGroupShared);
    bcmCheckError(rv, "Failed to sync bcmBstStatIdPriGroupShared stat");
  }
  if (syncQueue) {
    int rv = bcm_cosq_bst_stat_sync(
        hw_->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdIngPool);
    bcmCheckError(rv, "Failed to sync bcmBstStatIdIngPool stat");
  }
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

void BcmBstStatsMgr::getAndPublishGlobalWatermarks(
    const std::map<int, bcm_port_t>& itmToPortMap) {
  auto cosMgr = hw_->getCosMgr();
  uint64_t globalHeadroomWatermarkBytes{};
  uint64_t globalSharedWatermarkBytes{};
  for (auto it = itmToPortMap.begin(); it != itmToPortMap.end(); it++) {
    int itm = it->first;
    bcm_port_t bcmPortId = it->second;
    uint64_t maxGlobalHeadroomBytes =
        cosMgr->statGetExtended(
            itm, PortID(bcmPortId), -1, bcmBstStatIdHeadroomPool) *
        hw_->getMMUCellBytes();
    uint64_t maxGlobalSharedBytes =
        cosMgr->statGetExtended(
            itm, PortID(bcmPortId), -1, bcmBstStatIdIngPool) *
        hw_->getMMUCellBytes();
    publishGlobalWatermarks(itm, maxGlobalHeadroomBytes, maxGlobalSharedBytes);
    globalHeadroomWatermarkBytes =
        std::max(globalHeadroomWatermarkBytes, maxGlobalHeadroomBytes);
    globalSharedWatermarkBytes =
        std::max(globalSharedWatermarkBytes, maxGlobalSharedBytes);
  }
  auto ingressBufferPoolName =
      (*hw_->getPortTable()->begin()).second->getIngressBufferPoolName();
  globalHeadroomWatermarkBytes_[ingressBufferPoolName] =
      globalHeadroomWatermarkBytes;
  globalSharedWatermarkBytes_[ingressBufferPoolName] =
      globalSharedWatermarkBytes;
}

void BcmBstStatsMgr::createItmToPortMap(
    std::map<int, bcm_port_t>& itmToPortMap) const {
  if (itmToPortMap.size() == 0) {
    /*
     * ITM to port map not available, need to populate it first!
     * Global headroom/shared buffer stats are per ITM, but as
     * the counters are to be read per port, we need to keep track
     * of one port per ITM for which stats can be read. This mapping
     * is static and doesnt change.
     */
    for (const auto& entry : *hw_->getPortTable()) {
      BcmPort* bcmPort = entry.second;
      int itm = hw_->getPlatform()->getPortItm(bcmPort);
      if (itmToPortMap.find(itm) == itmToPortMap.end()) {
        itmToPortMap[itm] = bcmPort->getBcmPortId();
        XLOG(DBG2) << "ITM" << itm << " mapped to bcmport "
                   << itmToPortMap[itm];
      }
    }
  }
}

void BcmBstStatsMgr::updateStats() {
  syncStats();

  // Track if PG is enabled on any port in the system
  bool pgEnabled = false;
  auto qosSupported =
      hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS);
  for (const auto& entry : *hw_->getPortTable()) {
    BcmPort* bcmPort = entry.second;

    auto curPortStatsOptional = bcmPort->getPortStats();
    if (!curPortStatsOptional) {
      // ignore ports with no saved stats
      continue;
    }

    auto cosMgr = hw_->getCosMgr();

    std::map<int16_t, int64_t> queueId2WatermarkBytes;
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
            curPortStatsOptional->queueOutDiscardBytes_()->at(queue),
            bcmPort->getPlatformPort()->getEgressXPEs());
      }
    }
    bcmPort->setQueueWaterMarks(std::move(queueId2WatermarkBytes));

    // Get PG MAX headroom/shared stats
    if (bcmPort->isPortPgConfigured()) {
      bcm_port_t bcmPortId = bcmPort->getBcmPortId();
      uint64_t maxHeadroomBytes =
          cosMgr->statGet(PortID(bcmPortId), -1, bcmBstStatIdPriGroupHeadroom) *
          hw_->getMMUCellBytes();
      uint64_t maxSharedBytes =
          cosMgr->statGet(PortID(bcmPortId), -1, bcmBstStatIdPriGroupShared) *
          hw_->getMMUCellBytes();
      publishPgWatermarks(
          bcmPort->getPortName(), maxHeadroomBytes, maxSharedBytes);
      pgEnabled = true;
    }
  }

  if (pgEnabled) {
    /*
     * Ingress Traffic Manager(ITM) is part of the MMU. The ports in the
     * system are split between multiple ITMs and the ITMs has buffers
     * shared by all port which are part of the same ITM. The global
     * ingress buffer statistics are tracked per ITM and hence needs
     * to be fetched only once per ITM. This would mean we first find
     * the port->ITM mapping and then read the statistics once per ITM.
     */
    static std::map<int, bcm_port_t> itmToPortMap;
    createItmToPortMap(itmToPortMap);
    getAndPublishGlobalWatermarks(itmToPortMap);
  }

  // Get watermark stats for CPU queues
  if (qosSupported) {
    auto controlPlane = hw_->getControlPlane();
    HwPortStats cpuStats;
    controlPlane->updateQueueWatermarks(&cpuStats);
    for (const auto& [cosQ, stats] : *cpuStats.queueWatermarkBytes_()) {
      publishCpuQueueWatermark(cosQ, stats);
    }
  }

  getAndPublishDeviceWatermark();
}

void BcmBstStatsMgr::populateHighFrequencyBstPortStats(
    const PortID portId,
    const BcmPort* bcmPort,
    const HfPortStatsCollectionConfig& portStatsConfig,
    HwHighFrequencyStats& stats) const {
  if (!(portStatsConfig.includePgWatermark().value() ||
        portStatsConfig.includeQueueWatermark().value())) {
    return;
  }
  if (!bcmPort->isUp() || !bcmPort->isPortPgConfigured()) {
    return;
  }
  std::map<int16_t, int64_t> queueIdToWatermarkBytes;
  std::map<int16_t, int64_t> queueIdToPGWatermarkBytes;
  bcm_gport_t gport = bcmPort->getBcmGport();
  uint32_t options = BCM_COSQ_STAT_CLEAR;
  // A 1:1 mapping between the priority group and queue is assumed
  for (int16_t queue : kHfQueueIds) {
    std::vector<bcm_bst_stat_id_t> bstStatTypes{};
    if (portStatsConfig.includePgWatermark().value()) {
      if (bcmPort->isPortPgConfigured()) {
        bstStatTypes.push_back(bcmBstStatIdPriGroupShared);
      } else {
        XLOG(ERR) << "PG watermark stats requested for port " << portId
                  << ", but port is not configured for PG";
      }
    }
    if (portStatsConfig.includeQueueWatermark().value()) {
      bstStatTypes.push_back(bcmBstStatIdUcast);
    }
    std::vector<uint64_t> bstStatValues(bstStatTypes.size());
    auto rv = bcm_cosq_bst_stat_multi_get(
        hw_->getUnit(),
        gport,
        queue,
        options,
        static_cast<int>(bstStatTypes.size()),
        const_cast<bcm_bst_stat_id_t*>(bstStatTypes.data()),
        bstStatValues.data());
    bcmCheckError(
        rv, "Failed to get BST stat for port: ", portId, " cosq: ", queue);
    for (int i = 0; i < bstStatTypes.size(); i++) {
      if (bstStatTypes.at(i) == bcmBstStatIdPriGroupShared) {
        // The PG ID has a 1:1 mapping to the queue ID
        uint16_t pgId = queue;
        queueIdToPGWatermarkBytes[pgId] =
            bstStatValues.at(i) * hw_->getMMUCellBytes();
      } else if (bstStatTypes.at(i) == bcmBstStatIdUcast) {
        queueIdToWatermarkBytes[queue] =
            bstStatValues.at(i) * hw_->getMMUCellBytes();
      }
    }
  }
  stats.portStats()[bcmPort->getPortName()].pgSharedWatermarkBytes() =
      queueIdToPGWatermarkBytes;
  stats.portStats()[bcmPort->getPortName()].queueWatermarkBytes() =
      queueIdToWatermarkBytes;
}

void BcmBstStatsMgr::populateHighFrequencyBstStats(
    const HfStatsConfig& statsConfig,
    HwHighFrequencyStats& stats) const {
  syncHighFrequencyStats(statsConfig);
  switch (statsConfig.portStatsConfig()->getType()) {
    case HfPortStatsConfig::Type::allPortsConfig: {
      for (const auto& [portId, bcmPort] : *hw_->getPortTable()) {
        populateHighFrequencyBstPortStats(
            portId,
            bcmPort,
            statsConfig.portStatsConfig()->allPortsConfig().value(),
            stats);
      }
      break;
    }
    case HfPortStatsConfig::Type::filterConfig: {
      auto filterConfig = statsConfig.portStatsConfig()->filterConfig();
      for (const auto& [portId, bcmPort] : *hw_->getPortTable()) {
        auto it = filterConfig->find(bcmPort->getPortName());
        if (it == filterConfig->end()) {
          continue;
        }
        populateHighFrequencyBstPortStats(portId, bcmPort, it->second, stats);
      }
      break;
    }
    default: {
      break;
    }
  }
  static std::map<int, bcm_port_t> itmToPortMap;
  createItmToPortMap(itmToPortMap);
  if (statsConfig.includeDeviceWatermark().value()) {
    BcmCosManager* cosMgr = hw_->getCosMgr();
    for (int itm : kHfItms) {
      auto it = itmToPortMap.find(itm);
      if (it == itmToPortMap.end()) {
        XLOG(ERR) << "ITM" << itm << " not found in itmToPortMap";
        continue;
      }
      stats.itmPoolSharedWatermarkBytes()[itm] =
          cosMgr->statGetExtended(
              itm, PortID(it->second), -1, bcmBstStatIdIngPool) *
          hw_->getMMUCellBytes();
    }
  }
}

} // namespace facebook::fboss
