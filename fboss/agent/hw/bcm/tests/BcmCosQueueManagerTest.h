/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/PortQueue.h"

namespace facebook::fboss {

class BcmCosQueueManagerTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override = 0;

  virtual std::optional<cfg::PortQueue> getCfgQueue(int queueID) = 0;

  virtual QueueConfig getHwQueues() = 0;

  virtual const QueueConfig& getSwQueues() = 0;

  void checkCosQueueAPI();

  void checkDefaultCosqMatch(const std::shared_ptr<PortQueue>& queue);

  void checkConfSwHwMatch();

  std::shared_ptr<SwitchState> applyNewConfig(const cfg::SwitchConfig& config) {
    programmedCfg_ = config;
    return BcmTest::applyNewConfig(config);
  }

  cfg::SwitchConfig programmedCfg_;
};

} // namespace facebook::fboss
