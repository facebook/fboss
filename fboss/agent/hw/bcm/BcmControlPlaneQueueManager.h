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

class BcmControlPlaneQueueManager : public BcmCosQueueManager {
 public:
  // HACK: there seems to be some bugs in some queue apis related to
  // the number of queues. Specifically, if you use the cpu gport +
  // queue id to access cpu cosq, it rejects ids above 32. Also the
  // calls to get scheduling mode are inconsistent above queue id
  // 10. Since we don't use queues above 9, cap the number of
  // queues while we work through the issue + upgrade sdk.
  static constexpr auto kMaxMCQueueSize = 10;

  explicit BcmControlPlaneQueueManager(BcmSwitch* hw);

  ~BcmControlPlaneQueueManager() override;

  int getNumQueues(cfg::StreamType streamType) const override;

  bcm_gport_t getQueueGPort(cfg::StreamType streamType, bcm_cos_queue_t cosQ)
      const override;

  BcmPortQueueConfig getCurrentQueueSettings() const override;

  std::unique_ptr<PortQueue> getCurrentQueueSettings(
      cfg::StreamType streamType,
      bcm_cos_queue_t cosQ) const override;

  void program(const PortQueue& queue) override;

  const std::vector<BcmCosQueueCounterType>& getQueueCounterTypes()
      const override;

 protected:
  const PortQueue& getDefaultQueueSettings(
      cfg::StreamType streamType) const override;

 private:
  // Forbidden copy constructor and assignment operator
  BcmControlPlaneQueueManager(BcmControlPlaneQueueManager const&) = delete;
  BcmControlPlaneQueueManager& operator=(BcmControlPlaneQueueManager const&) =
      delete;
  BcmControlPlaneQueueManager(BcmControlPlaneQueueManager&&) = delete;
  BcmControlPlaneQueueManager& operator=(BcmControlPlaneQueueManager&&) =
      delete;

  std::pair<bcm_gport_t, bcm_cos_queue_t> getQueueStatIDPair(
      bcm_cos_queue_t cosQ,
      cfg::StreamType streamType) override;

  int maxCPUQueue_{0};
};

} // namespace facebook::fboss
