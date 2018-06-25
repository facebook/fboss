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

#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/hw/bcm/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/bcm/BcmCosQueueCounterType.h"

#include <map>
#include <string>
#include <vector>
#include <boost/container/flat_map.hpp>

extern "C" {
#include <opennsl/types.h>
}

namespace facebook { namespace fboss {
typedef std::map<opennsl_cos_queue_t, opennsl_gport_t> CosQGportMap;

struct CosQueueGports {
  opennsl_gport_t scheduler;
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

  BcmPortQueueConfig(QueueConfig unicast,
                     QueueConfig multicast)
    : unicast(std::move(unicast)),
      multicast(std::move(multicast)) {}
};

// Shared control types for both uc and mc queues
enum class BcmCosQueueControlType {
  ALPHA,
  RESERVED_BYTES,
  SHARED_BYTES
};


class BcmSwitch;

class BcmCosQueueManager {
public:
  BcmCosQueueManager(BcmSwitch* hw, const std::string& portName,
                     opennsl_gport_t portGport)
    : hw_(hw),
      portName_(portName),
      portGport_(portGport) {}

  virtual ~BcmCosQueueManager() = default;

  CosQueueGports* getCosQueueGports() {
    // TODO(joseph5wu) Deprecate this api by changing caller to get the cosq
    // manager and then call a new function to fill the queue gport list.
    return &cosQueueGports_;
  }

  virtual int getNumQueues(cfg::StreamType streamType) const = 0;

  opennsl_cos_queue_t getCosQueue(cfg::StreamType streamType,
                                  opennsl_gport_t gport) const;

  virtual opennsl_gport_t getQueueGPort(cfg::StreamType streamType,
                                        opennsl_cos_queue_t cosQ) const = 0;

  void setPortName(const std::string& portName) {
    portName_ = portName;
  }

  virtual BcmPortQueueConfig getCurrentQueueSettings() const = 0;

  virtual std::shared_ptr<PortQueue> getCurrentQueueSettings(
    cfg::StreamType streamType,
    opennsl_cos_queue_t cosQ) const = 0;

  virtual void program(const std::shared_ptr<PortQueue>& queue) = 0;

  struct QueueStatCounters {
    facebook::stats::MonotonicCounter* aggregated = nullptr;
    boost::container::flat_map<int, facebook::stats::MonotonicCounter*> queues;
  };
  const std::map<BcmCosQueueCounterType, QueueStatCounters>&
  getQueueCounters() const {
    return queueCounters_;
  }
  virtual const std::vector<BcmCosQueueCounterType>&
  getQueueCounterTypes() const = 0;

  void setupQueueCounters();
  void updateQueueStats(std::chrono::seconds now,
                        HwPortStats* portStats = nullptr);

protected:
  int getControlValue(cfg::StreamType streamType,
                      opennsl_gport_t gport,
                      opennsl_cos_queue_t cosQ,
                      BcmCosQueueControlType ctrlType) const;

  void programControlValue(cfg::StreamType streamType,
                           opennsl_gport_t gport,
                           opennsl_cos_queue_t cosQ,
                           BcmCosQueueControlType ctrlType,
                           int value);

  void getSchedulingAndWeight(opennsl_gport_t gport,
                              opennsl_cos_queue_t cosQ,
                              std::shared_ptr<PortQueue> queue) const;
  void programSchedulingAndWeight(opennsl_gport_t gport,
                                  opennsl_cos_queue_t cosQ,
                                  const std::shared_ptr<PortQueue>& queue);

  virtual void getReservedBytes(opennsl_gport_t gport,
                                opennsl_cos_queue_t cosQ,
                                std::shared_ptr<PortQueue> queue) const = 0;
  virtual void programReservedBytes(
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosQ,
    const std::shared_ptr<PortQueue>& queue) = 0;

  void getSharedBytes(opennsl_gport_t gport,
                      opennsl_cos_queue_t cosQ,
                      std::shared_ptr<PortQueue> queue) const;
  void programSharedBytes(opennsl_gport_t gport,
                          opennsl_cos_queue_t cosQ,
                          const std::shared_ptr<PortQueue>& queue);

  void getBandwidth(opennsl_gport_t gport,
                    opennsl_cos_queue_t cosQ,
                    std::shared_ptr<PortQueue> queue) const;
  void programBandwidth(opennsl_gport_t gport,
                        opennsl_cos_queue_t cosQ,
                        const std::shared_ptr<PortQueue>& queue);

  const BcmSwitch* hw_;
  // owner port name of this cosq manager
  std::string portName_;
  // owner port gport of this cosq manager
  opennsl_gport_t portGport_;
  CosQueueGports cosQueueGports_;

private:
  // Forbidden copy constructor and assignment operator
  BcmCosQueueManager(BcmCosQueueManager const &) = delete;
  BcmCosQueueManager& operator=(BcmCosQueueManager const &) = delete;

  void fillOrReplaceCounter(const BcmCosQueueCounterType& type,
                           QueueStatCounters& counters);

  void setupQueueCounter(const BcmCosQueueCounterType& type);

  void updateQueueAggregatedStat(const BcmCosQueueCounterType& type,
                                 facebook::stats::MonotonicCounter* counter,
                                 std::chrono::seconds now,
                                 HwPortStats* portStats = nullptr);

  virtual void updateQueueStat(int cosQ,
                               const BcmCosQueueCounterType& type,
                               facebook::stats::MonotonicCounter* counter,
                               std::chrono::seconds now,
                               HwPortStats* portStats = nullptr) = 0;

  std::map<BcmCosQueueCounterType, QueueStatCounters> queueCounters_;
};
}} //facebook::fboss
