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

#include "fboss/agent/hw/bcm/BcmCosQueueManager.h"

namespace facebook::fboss {

class BcmSwitch;

class BcmPortQueueManager : public BcmCosQueueManager {
 public:
  BcmPortQueueManager(
      BcmSwitch* hw,
      const std::string& portName,
      bcm_gport_t portGport)
      : BcmCosQueueManager(hw, portName, portGport) {}

  ~BcmPortQueueManager() {}

  int getNumQueues(cfg::StreamType streamType) const override;

  bcm_gport_t getQueueGPort(cfg::StreamType streamType, bcm_cos_queue_t cosQ)
      const override;

  BcmPortQueueConfig getCurrentQueueSettings() const override;

  void program(const PortQueue& queue) override;

  const std::vector<BcmCosQueueCounterType>& getQueueCounterTypes()
      const override;

  static int CosQToBcmInternalPriority(bcm_cos_queue_t cosQ);
  static bcm_cos_queue_t bcmInternalPriorityToCosQ(int prio);

 private:
  // Forbidden copy constructor and assignment operator
  BcmPortQueueManager(BcmPortQueueManager const&) = delete;
  BcmPortQueueManager& operator=(BcmPortQueueManager const&) = delete;

  const PortQueue& getDefaultQueueSettings(
      cfg::StreamType streamType) const override;

  std::unique_ptr<PortQueue> getCurrentQueueSettings(
      cfg::StreamType streamType,
      bcm_cos_queue_t cosQ) const override;

  void updateQueueStat(
      bcm_cos_queue_t cosQ,
      const BcmCosQueueCounterType& type,
      facebook::stats::MonotonicCounter* counter,
      std::chrono::seconds now,
      HwPortStats* portStats = nullptr) override;

  void getAlpha(bcm_gport_t gport, bcm_cos_queue_t cosQ, PortQueue* queue)
      const;
  void
  programAlpha(bcm_gport_t gport, bcm_cos_queue_t cosQ, const PortQueue& queue);

  void getAqms(bcm_gport_t gport, bcm_cos_queue_t cosQ, PortQueue* queue) const;

  void
  programAqms(bcm_gport_t gport, bcm_cos_queue_t cosQ, const PortQueue& queue);

  // if detection is null, the aqm for such behavior will be reset to default
  void programAqm(
      bcm_gport_t gport,
      bcm_cos_queue_t cosQ,
      cfg::QueueCongestionBehavior behavior,
      std::optional<cfg::QueueCongestionDetection> detection);

  void
  programTrafficClass(bcm_gport_t queueGport, bcm_cos_queue_t cosQ, int prio);
};

} // namespace facebook::fboss
