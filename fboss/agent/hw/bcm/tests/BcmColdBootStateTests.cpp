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

#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwVlanUtils.h"

namespace facebook::fboss {

TEST_F(BcmTest, chipColdBootDefaults) {
  auto setup = [] {};
  auto verify = [this] {
    // A single default VLAN should be configured
    auto vlans = getConfiguredVlans(getHwSwitch());
    EXPECT_EQ(vlans.size(), 1);
    // All ports should be part of this vlan
    auto portTable = getHwSwitch()->getPortTable();
    // Minus 1 for CPU port, which is also in default VLAN
    EXPECT_EQ(
        getVlanToNumPorts(getHwSwitch())[vlans[0]] - 1, portTable->size());

    // All ports should be down, and in default VLAN
    for (auto portIdAndPort : *portTable) {
      EXPECT_FALSE(portIdAndPort.second->isEnabled());
    }
  };
  setup();
  verify();
}

} // namespace facebook::fboss
