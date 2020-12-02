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

#include "fboss/agent/hw/bcm/BcmFlexCounter.h"

#include <memory>

namespace facebook::fboss {

class BcmSwitch;

class BcmEgressQueueFlexCounter : public BcmFlexCounter {
 public:
  BcmEgressQueueFlexCounter(BcmSwitch* hw, int numPorts, int numQueuesPerPort);
  ~BcmEgressQueueFlexCounter() = default;

 private:
  // Forbidden copy constructor and assignment operator
  BcmEgressQueueFlexCounter(BcmEgressQueueFlexCounter const&) = delete;
  BcmEgressQueueFlexCounter& operator=(BcmEgressQueueFlexCounter const&) =
      delete;

  BcmSwitch* hw_;
  int reservedNumQueuesPerPort_{0};
};

class BcmEgressQueueFlexCounterManager {
 public:
  explicit BcmEgressQueueFlexCounterManager(BcmSwitch* hw);

 private:
  // Forbidden copy constructor and assignment operator
  BcmEgressQueueFlexCounterManager(BcmEgressQueueFlexCounterManager const&) =
      delete;
  BcmEgressQueueFlexCounterManager& operator=(
      BcmEgressQueueFlexCounterManager const&) = delete;

  /*
   * Because cpu and regular port have different numbers of queues, which will
   * affect the flex counter index caculation, ref:setEgressQueueActionIndex(),
   * we need to create two counters respectively.
   */
  std::unique_ptr<BcmEgressQueueFlexCounter> cpuQueueFlexCounter_;
  std::unique_ptr<BcmEgressQueueFlexCounter> portQueueFlexCounter_;
};
} // namespace facebook::fboss
