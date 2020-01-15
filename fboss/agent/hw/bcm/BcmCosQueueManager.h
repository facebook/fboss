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

  virtual void updateQueueStat(
      int cosQ,
      const BcmCosQueueCounterType& type,
      facebook::stats::MonotonicCounter* counter,
      std::chrono::seconds now,
      HwPortStats* portStats = nullptr) = 0;

  std::map<BcmCosQueueCounterType, QueueStatCounters> queueCounters_;
};

} // namespace facebook::fboss
