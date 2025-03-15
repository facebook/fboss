/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"

DECLARE_int32(update_cable_length_stats_s);

namespace facebook::fboss {

void SaiSwitch::updateStatsImpl() {
  if (FLAGS_skip_stats_update_for_debug) {
    // Skip collecting any ASIC stats while debugs are in progress
    return;
  }

  auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  bool updateWatermarks = now - watermarkStatsUpdateTime_ >=
      FLAGS_update_watermark_stats_interval_s;
  if (updateWatermarks) {
    watermarkStatsUpdateTime_ = now;
  }

  // Space out VOQ stats update and watermark as much as possible - to avoid
  // stats collection blocking normal updates.
  // Allow frequent collection in test scenario, where the interval is set to 0.
  bool updateVoqStats = FLAGS_update_voq_stats_interval_s == 0 ||
      (now - voqStatsUpdateTime_ >= FLAGS_update_voq_stats_interval_s &&
       now - watermarkStatsUpdateTime_ >=
           (FLAGS_update_voq_stats_interval_s / 2));
  if (updateVoqStats) {
    voqStatsUpdateTime_ = now;
  }

  bool updateCableLengths =
      now - cableLengthStatsUpdateTime_ >= FLAGS_update_cable_length_stats_s;

  if (updateCableLengths) {
    cableLengthStatsUpdateTime_ = now;
  }

  {
    auto changePending = switchReachabilityChangePending_.wlock();
    if (*changePending > 0) {
      *changePending -= 1;
      auto reachabilityInfo = getSwitchReachabilityChange();
      switchReachabilityChangeProcessEventBase_.runInFbossEventBaseThread(
          [this, reachabilityInfo = std::move(reachabilityInfo)]() mutable {
            processSwitchReachabilityChange(reachabilityInfo);
          });
    }
  }

  int64_t missingCount = 0, mismatchCount = 0, bogusCount = 0;
  auto portsIter = concurrentIndices_->portSaiId2PortInfo.begin();
  std::map<PortID, multiswitch::FabricConnectivityDelta> connectivityDelta;
  while (portsIter != concurrentIndices_->portSaiId2PortInfo.end()) {
    {
      std::lock_guard<std::mutex> locked(saiSwitchMutex_);
      auto endpointOpt = managerTable_->portManager().getFabricConnectivity(
          portsIter->second.portID);
      if (endpointOpt.has_value()) {
        auto delta = fabricConnectivityManager_->processConnectivityInfoForPort(
            portsIter->second.portID, *endpointOpt);
        if (delta) {
          XLOG(DBG5) << "Connectivity delta found for port ID "
                     << portsIter->second.portID;
          connectivityDelta.insert({portsIter->second.portID, *delta});
        } else {
          XLOG(DBG5) << "No connectivity delta for port ID "
                     << portsIter->second.portID;
        }
        if (fabricConnectivityManager_->isConnectivityInfoMissing(
                portsIter->second.portID)) {
          missingCount++;
          XLOG(DBG5) << "Connectivity missing for port ID "
                     << portsIter->second.portID;
        }
        if (fabricConnectivityManager_->isConnectivityInfoMismatch(
                portsIter->second.portID)) {
          mismatchCount++;
          XLOG(DBG5) << "Connectivity mismatch for port ID "
                     << portsIter->second.portID;
        }
        if (fabricConnectivityManager_->isConnectivityInfoBogus(
                portsIter->second.portID)) {
          bogusCount++;
        }
      }
      managerTable_->portManager().updateStats(
          portsIter->second.portID, updateWatermarks, updateCableLengths);
    }
    ++portsIter;
  }

  getSwitchStats()->fabricConnectivityMissingCount(missingCount);
  getSwitchStats()->fabricConnectivityMismatchCount(mismatchCount);
  getSwitchStats()->fabricConnectivityBogusCount(bogusCount);

  auto sysPortsIter = concurrentIndices_->sysPortIds.begin();
  while (sysPortsIter != concurrentIndices_->sysPortIds.end()) {
    {
      std::lock_guard<std::mutex> locked(saiSwitchMutex_);
      managerTable_->systemPortManager().updateStats(
          sysPortsIter->second, updateWatermarks, updateVoqStats);
    }
    ++sysPortsIter;
  }
  auto lagsIter = concurrentIndices_->aggregatePortIds.begin();
  while (lagsIter != concurrentIndices_->aggregatePortIds.end()) {
    {
      std::lock_guard<std::mutex> locked(saiSwitchMutex_);
      managerTable_->lagManager().updateStats(lagsIter->second);
    }
    ++lagsIter;
  }
  if (platform_->getAsic()->isSupported(HwAsic::Feature::CPU_PORT)) {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    managerTable_->hostifManager().updateStats(
        updateWatermarks &&
        getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::CPU_QUEUE_WATERMARK_STATS));
  }

  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    managerTable_->bufferManager().updateStats();
  }
  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    HwResourceStatsPublisher(getPlatform()->getMultiSwitchStatsPrefix())
        .publish(hwResourceStats_);
  }
  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    managerTable_->aclTableManager().updateStats();
  }
  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    managerTable_->counterManager().updateStats();
  }
  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    managerTable_->switchManager().updateStats(updateWatermarks);
  }
  reportAsymmetricTopology();
  reportInterPortGroupCableSkew();
  if (!connectivityDelta.empty()) {
    XLOG(DBG2)
        << "Connectivity delta is not empty. Sending callback to SwSwitch";
    linkConnectivityChangeBottomHalfEventBase_.runInFbossEventBaseThread(
        [this, connectivityDelta = std::move(connectivityDelta)] {
          linkConnectivityChanged(connectivityDelta);
        });
  }
}

} // namespace facebook::fboss
