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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/platforms/BcmTestPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <string>

extern "C" {
#include <opennsl/port.h>
}

using std::make_shared;
using std::shared_ptr;
using std::string;

namespace {

using namespace facebook::fboss;

typedef std::vector<opennsl_vlan_data_t> VlanVector;

VlanVector listVlans(int unit) {
  opennsl_vlan_data_t* vlanList = nullptr;
  int vlanCount = 0;
  auto rv = opennsl_vlan_list(unit, &vlanList, &vlanCount);
  facebook::fboss::bcmCheckError(rv, "failed to list all VLANs");
  return VlanVector(vlanList, vlanList + vlanCount);
}
} // unnamed namespace

namespace facebook {
namespace fboss {

class BcmVlanTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(getHwSwitch(),
      masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }
};

TEST_F(BcmVlanTest, VlanInit) {
  auto setup = [=]() { return getProgrammedState(); };

  auto verify = [=]() {
    // Make sure there are still no VLANs except the default VLAN
    ASSERT_EQ(1, listVlans(getUnit()).size());
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmVlanTest, VlanApplyConfig) {
  auto setup = [=]() {
    return applyNewConfig(initialConfig());
  };

  auto verify = [=]() {
    // There should be 2 VLANs
    auto vlan_data_list = listVlans(getUnit());
    ASSERT_EQ(3, vlan_data_list.size());

    // There should be 2 ports
    auto ports = {masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
    for (auto port : ports) {
      ASSERT_TRUE(utility::portEnabled(getUnit(), port));
    }

    // kBaseVlanId should be associate with 2 ports
    std::map<int, int> vlan2PortCount = {
        {utility::kBaseVlanId, 2},
        {utility::kBaseVlanId + 1, 0},
    };
    for (auto& vlanData : vlan_data_list) {
      if (vlanData.vlan_tag == utility::kDefaultVlanId) {
        continue;
      }
      auto vlanItr = vlan2PortCount.find(vlanData.vlan_tag);
      ASSERT_TRUE(vlanItr != vlan2PortCount.end());
      int port_count;
      OPENNSL_PBMP_COUNT(vlanData.port_bitmap, port_count);
      EXPECT_EQ(vlanItr->second, port_count);

    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace fboss
} // namespace
