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

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwVlanUtils.h"

#include <string>

using std::make_shared;
using std::shared_ptr;
using std::string;

namespace facebook::fboss {

class HwVlanTest : public HwTest {
 protected:
  cfg::SwitchConfig config() const {
    return utility::twoL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }
};

TEST_F(HwVlanTest, VlanApplyConfig) {
  auto setup = [=]() { applyNewConfig(config()); };

  auto verify = [=]() {
    // There should be 2 VLANs
    auto vlan2NumPorts = getVlanToNumPorts(getHwSwitch());
    ASSERT_EQ(3, vlan2NumPorts.size());

    // kBaseVlanId should be associate with 2 ports
    std::map<int, int> vlan2PortCount = {
        {utility::kBaseVlanId, 2},
        {utility::kBaseVlanId + 1, 0},
    };
    for (auto& vlanAndNumPorts : vlan2NumPorts) {
      if (vlanAndNumPorts.first == VlanID{utility::kDefaultVlanId}) {
        continue;
      }
      auto vlanItr = vlan2PortCount.find(vlanAndNumPorts.first);
      ASSERT_TRUE(vlanItr != vlan2PortCount.end());
      EXPECT_EQ(vlanItr->second, vlanAndNumPorts.second);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
