/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/dataplane_tests/HwTestUtils.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"

#include <folly/logging/xlog.h>

#include <string>

namespace facebook::fboss::utility {

namespace {

template <typename ConditionFN, typename PortT, typename StatsGetFn>
bool waitPortStatsConditionImpl(
    ConditionFN conditionFn,
    const std::vector<PortT>& portIds,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry,
    StatsGetFn getHwPortStats) {
  auto newPortStats = getHwPortStats(portIds);
  while (retries--) {
    // TODO(borisb): exponential backoff!
    if (conditionFn(newPortStats)) {
      return true;
    }
    std::this_thread::sleep_for(msBetweenRetry);
    newPortStats = getHwPortStats(portIds);
  }
  XLOG(DBG3) << "Awaited port stats condition was never satisfied";
  return false;
}
} // namespace
bool waitPortStatsCondition(
    std::function<bool(const std::map<PortID, HwPortStats>&)> conditionFn,
    const std::vector<PortID>& portIds,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry,
    HwPortStatsFunc getHwPortStats) {
  return waitPortStatsConditionImpl(
      conditionFn, portIds, retries, msBetweenRetry, getHwPortStats);
}

bool waitSysPortStatsCondition(
    std::function<bool(const std::map<SystemPortID, HwSysPortStats>&)>
        conditionFn,
    const std::vector<SystemPortID>& portIds,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry,
    HwSysPortStatsFunc getHwPortStats) {
  return waitPortStatsConditionImpl(
      conditionFn, portIds, retries, msBetweenRetry, getHwPortStats);
}

bool waitForAnyPorAndQueutOutBytesIncrement(
    HwSwitch* hwSwitch,
    const std::map<PortID, HwPortStats>& originalPortStats,
    const std::vector<PortID>& portIds,
    HwPortStatsFunc getHwPortStats) {
  auto queueStatsSupported =
      hwSwitch->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS);
  auto conditionFn = [&originalPortStats,
                      queueStatsSupported](const auto& newPortStats) {
    for (const auto& [portId, portStat] : originalPortStats) {
      auto newPortStatItr = newPortStats.find(portId);
      if (newPortStatItr != newPortStats.end()) {
        if (*newPortStatItr->second.outBytes_() > portStat.outBytes_()) {
          // Wait for queue stat increment if queues are supported
          // on this platform
          if (!queueStatsSupported ||
              std::any_of(
                  portStat.queueOutBytes_()->begin(),
                  portStat.queueOutBytes_()->end(),
                  [newPortStatItr](auto queueAndBytes) {
                    auto [qid, oldQbytes] = queueAndBytes;
                    const auto newQueueStats =
                        newPortStatItr->second.queueOutBytes_();
                    return newQueueStats->find(qid)->second > oldQbytes;
                  })) {
            return true;
          }
        }
      }
    }
    XLOG(DBG3) << "No port stats increased yet";
    return false;
  };
  return waitPortStatsCondition(
      conditionFn, portIds, 20, std::chrono::milliseconds(20), getHwPortStats);
}

bool waitForAnyVoQOutBytesIncrement(
    HwSwitch* hwSwitch,
    const std::map<SystemPortID, HwSysPortStats>& originalPortStats,
    const std::vector<SystemPortID>& portIds,
    HwSysPortStatsFunc getHwPortStats) {
  if (!hwSwitch->getPlatform()->getAsic()->isSupported(HwAsic::Feature::VOQ)) {
    throw FbossError("VOQs are unsupported on platform");
  }
  auto conditionFn = [&originalPortStats](const auto& newPortStats) {
    for (const auto& [portId, portStat] : originalPortStats) {
      auto newPortStatItr = newPortStats.find(portId);
      if (newPortStatItr != newPortStats.end()) {
        if (std::any_of(
                portStat.queueOutBytes_()->begin(),
                portStat.queueOutBytes_()->end(),
                [newPortStatItr](auto queueAndBytes) {
                  auto [qid, oldQbytes] = queueAndBytes;
                  const auto newQueueStats =
                      newPortStatItr->second.queueOutBytes_();
                  return newQueueStats->find(qid)->second > oldQbytes;
                })) {
          return true;
        }
      }
    }
    XLOG(DBG3) << "No port stats increased yet";
    return false;
  };
  return waitSysPortStatsCondition(
      conditionFn, portIds, 20, std::chrono::milliseconds(20), getHwPortStats);
}

bool ensureSendPacketSwitched(
    HwSwitch* hwSwitch,
    std::unique_ptr<TxPacket> pkt,
    const std::vector<PortID>& portIds,
    HwPortStatsFunc getHwPortStats) {
  auto originalPortStats = getHwPortStats(portIds);
  bool result = hwSwitch->sendPacketSwitchedSync(std::move(pkt));
  return result &&
      waitForAnyPorAndQueutOutBytesIncrement(
             hwSwitch, originalPortStats, portIds, getHwPortStats);
}

bool ensureSendPacketOutOfPort(
    HwSwitch* hwSwitch,
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    const std::vector<PortID>& ports,
    HwPortStatsFunc getHwPortStats,
    std::optional<uint8_t> queue) {
  auto originalPortStats = getHwPortStats(ports);
  bool result =
      hwSwitch->sendPacketOutOfPortSync(std::move(pkt), portID, queue);
  return result &&
      waitForAnyPorAndQueutOutBytesIncrement(
             hwSwitch, originalPortStats, ports, getHwPortStats);
}

} // namespace facebook::fboss::utility
