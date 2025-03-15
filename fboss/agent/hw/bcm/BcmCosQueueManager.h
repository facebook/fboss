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

#include "common/stats/MonotonicCounter.h"

#include "fboss/agent/hw/bcm/BcmCosQueueCounterType.h"
#include "fboss/agent/hw/bcm/BcmEgressQueueFlexCounter.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/state/PortQueue.h"

#include <boost/container/flat_map.hpp>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <bcm/types.h>
}

namespace facebook::fboss {

typedef std::map<bcm_cos_queue_t, bcm_gport_t> CosQGportMap;

struct CosQueueGports {
  bcm_gport_t scheduler;
  CosQGportMap unicast;
  CosQGportMap multicast;
};

/**
 * TODO(joseph5wu) Will move this unicast/multicast combined data struct to
 * S/W side, so we can keep the genericity from S/W to H/W.
 */
struct BcmPortQueueConfig {
  QueueConfig unicast;
  QueueConfig multicast;

  BcmPortQueueConfig(QueueConfig unicast, QueueConfig multicast)
      : unicast(std::move(unicast)), multicast(std::move(multicast)) {}
};

// Shared control types for both uc and mc queues
enum class BcmCosQueueControlType { ALPHA, RESERVED_BYTES, SHARED_BYTES };

class BcmSwitch;

class BcmCosQueueManager {
 public:
  BcmCosQueueManager(
      BcmSwitch* hw,
      const std::string& portName,
      bcm_gport_t portGport);

  virtual ~BcmCosQueueManager() = default;

  CosQueueGports* getCosQueueGports() {
    // TODO(joseph5wu) Deprecate this api by changing caller to get the cosq
    // manager and then call a new function to fill the queue gport list.
    return &cosQueueGports_;
  }

  virtual int getNumQueues(cfg::StreamType streamType) const = 0;

  int getReservedBytes(
      cfg::StreamType streamType,
      bcm_gport_t gport,
      const bcm_cos_queue_t cosQ);

  bcm_cos_queue_t getCosQueue(cfg::StreamType streamType, bcm_gport_t gport)
      const;

  virtual bcm_gport_t getQueueGPort(
      cfg::StreamType streamType,
      bcm_cos_queue_t cosQ) const = 0;

  void setPortName(const std::string& portName) {
    portName_ = portName;
  }

  virtual BcmPortQueueConfig getCurrentQueueSettings() const = 0;

  virtual void program(const PortQueue& queue) = 0;

  std::set<bcm_cos_queue_t> getNamedQueues(cfg::StreamType streamType) const;

  struct QueueStatCounters {
    std::unique_ptr<facebook::stats::MonotonicCounter> aggregated = nullptr;
    boost::container::
        flat_map<int, std::unique_ptr<facebook::stats::MonotonicCounter>>
            queues;
  };
  const std::map<BcmCosQueueCounterType, QueueStatCounters>& getQueueCounters()
      const {
    return queueCounters_;
  }

  std::map<int32_t, int64_t> getQueueStats(BcmCosQueueStatType statType);

  virtual const std::vector<BcmCosQueueCounterType>& getQueueCounterTypes()
      const = 0;

  void setupQueueCounters(
      const std::optional<QueueConfig>& queueConfig = std::nullopt);
  void destroyQueueCounters();
  void updateQueueStats(
      std::chrono::seconds now,
      HwPortStats* portStats = nullptr);

  void getCosQueueGportsFromHw();
  bcm_gport_t getPortGport() const {
    return portGport_;
  }

  const BcmSwitch* getBcmSwitch() const {
    return hw_;
  }

 protected:
  void getSchedulingAndWeight(
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      PortQueue* queue) const;
  void programSchedulingAndWeight(
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      const PortQueue& queue);

  int getControlValue(
      cfg::StreamType streamType,
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      BcmCosQueueControlType ctrlType) const;

  void programControlValue(
      cfg::StreamType streamType,
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      BcmCosQueueControlType ctrlType,
      int value);

  void getReservedBytes(
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      PortQueue* queue) const;
  void programReservedBytes(
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      const PortQueue& queue);

  void getSharedBytes(bcm_gport_t gport, bcm_cos_queue_t cosQ, PortQueue* queue)
      const;
  void programSharedBytes(
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      const PortQueue& queue);

  void getBandwidth(bcm_gport_t gport, bcm_cos_queue_t cosQ, PortQueue* queue)
      const;
  void programBandwidth(
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      const PortQueue& queue);

  bool isEgressDynamicSharedEnabled(
      cfg::StreamType streamType,
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ) const;
  void programEgressDynamicSharedEnabled(
      cfg::StreamType streamType,
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ) const;

  void getAlpha(bcm_gport_t gport, bcm_cos_queue_t cosQ, PortQueue* queue)
      const;
  void
  programAlpha(bcm_gport_t gport, bcm_cos_queue_t cosQ, const PortQueue& queue);

  void updateNamedQueue(const PortQueue& queue);

  virtual const PortQueue& getDefaultQueueSettings(
      cfg::StreamType streamType) const = 0;

  const BcmSwitch* hw_;
  // owner port name of this cosq manager
  std::string portName_;
  // owner port gport of this cosq manager
  bcm_gport_t portGport_;
  CosQueueGports cosQueueGports_;

 private:
  // Forbidden copy constructor and assignment operator
  BcmCosQueueManager(BcmCosQueueManager const&) = delete;
  BcmCosQueueManager& operator=(BcmCosQueueManager const&) = delete;

  virtual std::unique_ptr<PortQueue> getCurrentQueueSettings(
      cfg::StreamType streamType,
      bcm_cos_queue_t cosQ) const = 0;

  void fillOrReplaceCounter(
      const BcmCosQueueCounterType& type,
      QueueStatCounters& counters,
      const std::optional<QueueConfig>& queueConfig = std::nullopt);

  void setupQueueCounter(
      const BcmCosQueueCounterType& type,
      const std::optional<QueueConfig>& queueConfig = std::nullopt);

  void updateQueueAggregatedStat(
      const BcmCosQueueCounterType& type,
      facebook::stats::MonotonicCounter* counter,
      std::chrono::seconds now,
      HwPortStats* portStats = nullptr);

  void updateQueueStat(
      bcm_cos_queue_t cosQ,
      BcmCosQueueStatType statType,
      uint64_t value,
      facebook::stats::MonotonicCounter* counter,
      std::chrono::seconds now,
      HwPortStats* portStats = nullptr);

  uint64_t getQueueStatValue(
      int unit,
      bcm_gport_t queueGport,
      bcm_cos_queue_t queueID,
      const std::string& portName,
      int queueNum,
      const BcmCosQueueCounterType& type,
      const BcmEgressQueueTrafficCounterStats& flexCtrStats);

  virtual std::pair<bcm_gport_t, bcm_cos_queue_t> getQueueStatIDPair(
      bcm_cos_queue_t cosQ,
      cfg::StreamType streamType) = 0;

  std::map<cfg::StreamType, std::set<bcm_cos_queue_t>> namedQueues_;
  std::map<BcmCosQueueCounterType, QueueStatCounters> queueCounters_;
  folly::Synchronized<BcmEgressQueueTrafficCounterStats> queueFlexCounterStats_;
};

} // namespace facebook::fboss
