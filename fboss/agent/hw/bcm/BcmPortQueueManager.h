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

#include <map>
#include <string>
#include <vector>

extern "C" {
#include <opennsl/types.h>
}

namespace facebook { namespace fboss {

class BcmSwitch;

struct CosQueueGports {
  opennsl_gport_t scheduler;
  std::vector<opennsl_gport_t> unicast;
  std::vector<opennsl_gport_t> multicast;
};

/**
 * TODO(joseph5wu) Will move this unicast/multicast combined data struct to
 * S/W side, so we can keep the genericity from S/W to H/W.
 */
struct BcmPortQueueConfig {
  QueueConfig unicast;
  QueueConfig multicast;

  BcmPortQueueConfig(QueueConfig unicast, QueueConfig multicast)
    : unicast(std::move(unicast)),
      multicast(std::move(multicast)) {}
};

/*
 * Since bcm stats type is not OpenNSL supported, define our enums instead.
 */
enum BcmPortQueueStatType {
  DROPPED_PACKETS,
  DROPPED_BYTES,
  OUT_PACKETS,
  OUT_BYTES
};
enum BcmPortQueueCounterScope {
  QUEUES,                // only collect each single queue stat
  AGGREGATED,            // only collect aggregated queue stat
  QUEUES_AND_AGGREGATED  // collect both single queue and aggregated stats
};
struct BcmPortQueueCounterType {
  cfg::StreamType streamType;
  BcmPortQueueStatType statType;
  BcmPortQueueCounterScope scope;
  folly::StringPiece name;

  bool operator<(const BcmPortQueueCounterType& t) const {
    return std::tie(streamType, statType, scope, name) <
           std::tie(t.streamType, t.statType, t.scope, t.name);
  }

  bool isScopeQueues() const {
    return scope == BcmPortQueueCounterScope::QUEUES ||
           scope == BcmPortQueueCounterScope::QUEUES_AND_AGGREGATED;
  }
  bool isScopeAggregated() const {
    return scope == BcmPortQueueCounterScope::AGGREGATED ||
           scope == BcmPortQueueCounterScope::QUEUES_AND_AGGREGATED;
  }
};

class BcmPortQueueManager {
public:
  BcmPortQueueManager(BcmSwitch* hw, const std::string& portName,
                      opennsl_gport_t portGport)
    : hw_(hw),
      portName_(portName),
      portGport_(portGport) {}

  CosQueueGports* getCosQueueGports() {
    return &cosQueueGports_;
  }

  opennsl_gport_t getQueueGPort(const std::shared_ptr<PortQueue>& queue) {
    return getQueueGPort(queue->getStreamType(), queue->getID());
  }

  struct QueueStatCounters {
    facebook::stats::MonotonicCounter* aggregated = nullptr;
    std::vector<facebook::stats::MonotonicCounter> queues;
  };
  const std::map<BcmPortQueueCounterType, QueueStatCounters>&
  getQueueCounters () const {
    return queueCounters_;
  }

  int getQueueSize(cfg::StreamType streamType);

  void setPortName(const std::string& portName) {
    portName_ = portName;
  }

  BcmPortQueueConfig getCurrentQueueSettings();

  std::shared_ptr<PortQueue> getCurrentQueueSettings(
    cfg::StreamType streamType, opennsl_gport_t queueGport, int queueIdx);

  void setupQueue(const std::shared_ptr<PortQueue>& queue);

  void setupQueueCounters(const std::vector<BcmPortQueueCounterType>& types);

  void updateQueueStats(std::chrono::seconds now,
                        HwPortStats* portStats = nullptr);


private:
  // no copy or assignment
  BcmPortQueueManager(BcmPortQueueManager const &) = delete;
  BcmPortQueueManager& operator=(BcmPortQueueManager const &) = delete;

  opennsl_gport_t getQueueGPort(cfg::StreamType streamType, int queueID);

  void fillOrReplaceCounter(
    const BcmPortQueueCounterType& type, QueueStatCounters& counters);

  const BcmSwitch* hw_;
  // portName_ is only used for logging better infos
  std::string portName_;
  opennsl_gport_t portGport_;
  CosQueueGports cosQueueGports_;
  std::map<BcmPortQueueCounterType, QueueStatCounters> queueCounters_;
};
}} // facebook::fboss
