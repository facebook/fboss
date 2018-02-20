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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/state/ControlPlane.h"

#include <boost/container/flat_map.hpp>
#include <vector>

extern "C" {
#include <opennsl/types.h>
}

using facebook::stats::MonotonicCounter;

namespace facebook { namespace fboss {

class BcmSwitch;
class PortQueue;

class BcmControlPlane {
public:
  BcmControlPlane(BcmSwitch* hw);

  ~BcmControlPlane() {}

  /*
   * Getters.
   */
  opennsl_gport_t getCPUGPort() const {
    return gport_;
  }
  CosQueueGports* getCosQueueGports() {
    return &cosQueueGports_;
  }

  /**
   * Set up the corresponding cos queue for ControlPlane
   */
  void setupQueue(const std::shared_ptr<PortQueue>& queue);

  /**
   * Set up the rx packet reason to queue mapping
   */
  void setupRxReasonToQueue(const ControlPlane::RxReasonToQueue& reasonToQueue);

  /**
   * TODO(joseph5wu) Temporarily keep this functions cause we haven't supported
   * applying ControlPlane queue settings from config
   */
  void setupQueueCounters();

  void updateQueueCounters();

private:
  // no copy or assignment
  BcmControlPlane(BcmControlPlane const &) = delete;
  BcmControlPlane& operator=(BcmControlPlane const &) = delete;

  BcmSwitch* hw_{nullptr};
  // Broadcom global port number
  const opennsl_gport_t gport_;
  CosQueueGports cosQueueGports_;

  struct QueueCounter {
    bool isDropCounter;
    MonotonicCounter counter;
  };
  // key=queue id
  boost::container::flat_map<opennsl_cos_queue_t,
                             std::vector<QueueCounter>> queueCounters_;
};
}} // facebook::fboss
