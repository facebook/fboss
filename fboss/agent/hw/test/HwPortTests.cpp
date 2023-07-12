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
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        false /*interfaceHasSubnet*/,
        false, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true);
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

TEST_F(HwPortTest, PortLoopbackMode) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (const auto& loopbackMode : getAsic()->desiredLoopbackModes()) {
      std::map<PortID, int> port2LoopbackMode = {
          {PortID(masterLogicalPortIds({loopbackMode.first})[0]),
           utility::getLoopbackMode(loopbackMode.second)}};
      utility::assertPortsLoopbackMode(getHwSwitch(), port2LoopbackMode);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
