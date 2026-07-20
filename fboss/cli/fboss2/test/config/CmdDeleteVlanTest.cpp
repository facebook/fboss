/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <string>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/commands/config/vlan/VlanManager.h"
#include "fboss/cli/fboss2/commands/delete/vlan/CmdDeleteVlan.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Seed covers every deleteVlan branch:
//   100 - barebone VLAN + its paired interface (fboss100, no IPs): deletable
//   200 - untagged ingress VLAN for port eth1/1/1
//   300 - has a switchport member (VlanPort logicalPort 2)
//   400 - backs a routed SVI (interface with an IP address)
//   1   - the global default VLAN
class CmdDeleteVlanTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteVlanTestFixture()
      : CmdConfigTestBase(
            "fboss2_vlan_delete_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "defaultVlan": 1,
    "vlans": [
      {"id": 1, "name": "default"},
      {"id": 100, "name": "Vlan100"},
      {"id": 200, "name": "Vlan200"},
      {"id": 300, "name": "Vlan300"},
      {"id": 400, "name": "Vlan400"}
    ],
    "interfaces": [
      {"intfID": 100, "vlanID": 100, "name": "fboss100", "ipAddresses": []},
      {"intfID": 400, "vlanID": 400, "name": "fboss400",
       "ipAddresses": ["10.0.4.1/24"]}
    ],
    "ports": [
      {"logicalID": 1, "name": "eth1/1/1", "ingressVlan": 200}
    ],
    "vlanPorts": [
      {"vlanID": 300, "logicalPort": 2}
    ],
    "staticMacAddrs": [
      {"vlanID": 100, "macAddress": "02:00:00:00:01:00",
       "egressLogicalPortID": 1}
    ]
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete vlan";

  cfg::SwitchConfig& swConfig() {
    return *ConfigSession::getInstance().getAgentConfig().sw();
  }
};

// ============================================================================
// VlanManager::deleteVlan — refuse matrix + not-found
// ============================================================================

TEST_F(CmdDeleteVlanTestFixture, refuseNotFound) {
  setupTestableConfigSession(cmdPrefix_, "999");
  EXPECT_THROW(VlanManager::deleteVlan(swConfig(), VlanID(999)), FbossError);
}

TEST_F(CmdDeleteVlanTestFixture, refuseDefaultVlan) {
  setupTestableConfigSession(cmdPrefix_, "1");
  EXPECT_THROW(VlanManager::deleteVlan(swConfig(), VlanID(1)), FbossError);
}

TEST_F(CmdDeleteVlanTestFixture, refuseIngressVlanPort) {
  setupTestableConfigSession(cmdPrefix_, "200");
  try {
    VlanManager::deleteVlan(swConfig(), VlanID(200));
    FAIL() << "expected FbossError";
  } catch (const FbossError& e) {
    EXPECT_THAT(std::string(e.what()), HasSubstr("eth1/1/1"));
  }
}

TEST_F(CmdDeleteVlanTestFixture, refuseSwitchportMember) {
  setupTestableConfigSession(cmdPrefix_, "300");
  EXPECT_THROW(VlanManager::deleteVlan(swConfig(), VlanID(300)), FbossError);
}

TEST_F(CmdDeleteVlanTestFixture, refuseSviWithIp) {
  setupTestableConfigSession(cmdPrefix_, "400");
  EXPECT_THROW(VlanManager::deleteVlan(swConfig(), VlanID(400)), FbossError);
}

// ============================================================================
// deleteVlan — happy path removes VLAN and its barebone interface
// ============================================================================

TEST_F(CmdDeleteVlanTestFixture, deleteUnreferencedRemovesVlanAndBareboneIntf) {
  setupTestableConfigSession(cmdPrefix_, "100");
  VlanManager::deleteVlan(swConfig(), VlanID(100));

  const auto& vlans = *swConfig().vlans();
  EXPECT_TRUE(std::none_of(vlans.begin(), vlans.end(), [](const auto& v) {
    return *v.id() == 100;
  }));
  // The paired barebone interface (vlanID 100, no IPs) is removed too.
  const auto& intfs = *swConfig().interfaces();
  EXPECT_TRUE(std::none_of(intfs.begin(), intfs.end(), [](const auto& i) {
    return *i.vlanID() == 100;
  }));
  // The SVI-backed VLAN's interface is untouched.
  EXPECT_TRUE(std::any_of(intfs.begin(), intfs.end(), [](const auto& i) {
    return *i.vlanID() == 400;
  }));
  // The VLAN's static MAC entry is cascade-removed (child object).
  ASSERT_TRUE(swConfig().staticMacAddrs().has_value());
  const auto& macs = *swConfig().staticMacAddrs();
  EXPECT_TRUE(std::none_of(macs.begin(), macs.end(), [](const auto& e) {
    return *e.vlanID() == 100;
  }));
}

// ============================================================================
// queryClient — exercises the handler + saveConfig on the happy path
// ============================================================================

TEST_F(CmdDeleteVlanTestFixture, queryClientDeletesVlan) {
  setupTestableConfigSession(cmdPrefix_, "100");
  CmdDeleteVlan cmd;
  HostInfo hostInfo("testhost");
  VlanId arg({"100"});

  auto result = cmd.queryClient(hostInfo, arg);
  EXPECT_THAT(result, HasSubstr("100"));

  const auto& vlans = *swConfig().vlans();
  EXPECT_TRUE(std::none_of(vlans.begin(), vlans.end(), [](const auto& v) {
    return *v.id() == 100;
  }));
}

TEST_F(CmdDeleteVlanTestFixture, queryClientRefusesReferenced) {
  setupTestableConfigSession(cmdPrefix_, "200");
  CmdDeleteVlan cmd;
  HostInfo hostInfo("testhost");
  VlanId arg({"200"});

  EXPECT_THROW(cmd.queryClient(hostInfo, arg), FbossError);
}

} // namespace facebook::fboss
