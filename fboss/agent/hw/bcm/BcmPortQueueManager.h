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
                                int queueIdx) const override;

  BcmPortQueueConfig getCurrentQueueSettings() const override;

  std::shared_ptr<PortQueue> getCurrentQueueSettings(
    cfg::StreamType streamType,
    int queueIdx) const override;

  void program(const std::shared_ptr<PortQueue>& queue) override;

  const std::vector<BcmCosQueueCounterType>&
  getQueueCounterTypes() const override;

protected:
  void getReservedBytes(opennsl_gport_t gport,
                        int queueIdx,
                        std::shared_ptr<PortQueue> queue) const override;
  void programReservedBytes(opennsl_gport_t gport,
                            int queueIdx,
                            const std::shared_ptr<PortQueue>& queue) override;

private:
  // Forbidden copy constructor and assignment operator
  BcmPortQueueManager(BcmPortQueueManager const &) = delete;
  BcmPortQueueManager& operator=(BcmPortQueueManager const &) = delete;

  void updateQueueStat(int queueIdx,
                       const BcmCosQueueCounterType& type,
                       facebook::stats::MonotonicCounter* counter,
                       std::chrono::seconds now,
                       HwPortStats* portStats = nullptr) override;

   void getAlpha(opennsl_gport_t gport,
                 int queueIdx,
                 std::shared_ptr<PortQueue> queue) const;
   void programAlpha(opennsl_gport_t gport,
                     int queueIdx,
                     const std::shared_ptr<PortQueue>& queue);

   void getAqms(opennsl_gport_t gport,
                int queueIdx,
                std::shared_ptr<PortQueue> queue) const;
   void programAqms(opennsl_gport_t gport,
                    int queueIdx,
                    const std::shared_ptr<PortQueue>& queue);
};
}} // facebook::fboss
