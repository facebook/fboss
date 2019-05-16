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

namespace facebook { namespace fboss {

class BcmSwitch;

class BcmPortQueueManager : public BcmCosQueueManager {
public:
  BcmPortQueueManager(
      BcmSwitch* hw,
      const std::string& portName,
      opennsl_gport_t portGport)
    : BcmCosQueueManager(hw, portName, portGport) {}

  ~BcmPortQueueManager() {}

  int getNumQueues(cfg::StreamType streamType) const override;

  opennsl_gport_t getQueueGPort(cfg::StreamType streamType,
                                opennsl_cos_queue_t cosQ) const override;

  BcmPortQueueConfig getCurrentQueueSettings() const override;

  void program(const PortQueue& queue) override;

  const std::vector<BcmCosQueueCounterType>&
  getQueueCounterTypes() const override;

  static int CosQToBcmInternalPriority(opennsl_cos_queue_t cosQ);
  static opennsl_cos_queue_t bcmInternalPriorityToCosQ(int prio);

private:
  // Forbidden copy constructor and assignment operator
  BcmPortQueueManager(BcmPortQueueManager const &) = delete;
  BcmPortQueueManager& operator=(BcmPortQueueManager const &) = delete;

  const PortQueue& getDefaultQueueSettings(
    cfg::StreamType streamType) const override;

  std::unique_ptr<PortQueue> getCurrentQueueSettings(
    cfg::StreamType streamType,
    opennsl_cos_queue_t cosQ) const override;

  void updateQueueStat(opennsl_cos_queue_t cosQ,
                       const BcmCosQueueCounterType& type,
                       facebook::stats::MonotonicCounter* counter,
                       std::chrono::seconds now,
                       HwPortStats* portStats = nullptr) override;

   void getAlpha(opennsl_gport_t gport,
                 opennsl_cos_queue_t cosQ,
                 PortQueue* queue) const;
   void programAlpha(opennsl_gport_t gport,
                     opennsl_cos_queue_t cosQ,
                     const PortQueue& queue);

   void getAqms(
       opennsl_gport_t gport,
       opennsl_cos_queue_t cosQ,
       PortQueue* queue) const;

   void programAqms(
       opennsl_gport_t gport,
       opennsl_cos_queue_t cosQ,
       const PortQueue& queue);

   // if detection is null, the aqm for such behavior will be reset to default
   void programAqm(
       opennsl_gport_t gport,
       opennsl_cos_queue_t cosQ,
       cfg::QueueCongestionBehavior behavior,
       folly::Optional<cfg::QueueCongestionDetection> detection);

   void programInternalPriorityToCosQ(
       opennsl_gport_t queueGport,
       opennsl_cos_queue_t cosQ,
       int prio);
};
}} // facebook::fboss
