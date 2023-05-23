/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTest.h"

#include <folly/Random.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <fb303/ServiceData.h>

namespace facebook::fboss {

class HwPortTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }
};

TEST_F(HwPortTest, PortLoopbackMode) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    auto portId = masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0];
    auto portCfg = utility::findCfgPort(newCfg, portId);
    portCfg->loopbackMode() = getAsic()->desiredLoopbackMode();
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    std::map<PortID, int> port2LoopbackMode = {
        {PortID(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]),
         utility::getLoopbackMode(getAsic()->desiredLoopbackMode())}};
    utility::assertPortsLoopbackMode(getHwSwitch(), port2LoopbackMode);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
