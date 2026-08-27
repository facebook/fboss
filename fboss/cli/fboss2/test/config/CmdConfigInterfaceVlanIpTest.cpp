// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Conv.h>
#include <folly/String.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

// IP add/remove on VLAN SVIs, addressed as "vlan<id>". The name resolves to
// the interface whose vlanID matches, so SVIs are reachable even though their
// interface name is unset or auto-generated (fboss<id>). Seed:
//   2001 - SVI-backed VLAN: interface fboss2001 (vlanID 2001) with one v4 and
//          one v6 address
//   2002 - VLAN with no interface: SVI address commands must refuse
//   2003 - SVI whose interface has NO name: the motivating case, reachable
//          only through the vlanID
//   2004 - VLAN with two interfaces: "vlan2004" is ambiguous and must refuse
//   eth1/1/1 - port-backed L3 interface: existing behavior must not change
class CmdConfigInterfaceVlanIpTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceVlanIpTestFixture()
      : CmdConfigTestBase(
            "config_interface_vlan_ip_test_%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "vlans": [
      {"id": 2001, "name": "Vlan2001"},
      {"id": 2002, "name": "Vlan2002"},
      {"id": 2003, "name": "Vlan2003"},
      {"id": 2004, "name": "Vlan2004"}
    ],
    "interfaces": [
      {"intfID": 2001, "vlanID": 2001, "name": "fboss2001", "routerID": 0,
       "ipAddresses": ["10.0.1.1/24", "2001:db8:1::1/64"]},
      {"intfID": 2003, "vlanID": 2003, "routerID": 0, "ipAddresses": []},
      {"intfID": 2004, "vlanID": 2004, "routerID": 0, "ipAddresses": []},
      {"intfID": 2005, "vlanID": 2004, "routerID": 0, "ipAddresses": []},
      {"intfID": 1, "vlanID": 1, "portID": 1, "name": "eth1/1/1",
       "routerID": 0, "ipAddresses": []}
    ],
    "ports": [
      {"logicalID": 1, "name": "eth1/1/1", "ingressVlan": 1}
    ],
    "vlanPorts": [
      {"vlanID": 2001, "logicalPort": 2}
    ]
  }
})") {}

 protected:
  // Interfaces in the seed config above; the "nothing was created" assertions
  // compare against it.
  static constexpr size_t kSeedInterfaces = 5;

  cfg::SwitchConfig& swConfig() {
    return *ConfigSession::getInstance().getAgentConfig().sw();
  }

  cfg::Interface& interfaceById(int32_t intfID) {
    auto& intfs = *swConfig().interfaces();
    auto it = std::find_if(intfs.begin(), intfs.end(), [intfID](const auto& i) {
      return *i.intfID() == intfID;
    });
    if (it == intfs.end()) {
      throw std::runtime_error(
          folly::to<std::string>("test config is missing interface ", intfID));
    }
    return *it;
  }

  cfg::Interface& sviInterface() {
    return interfaceById(2001);
  }

  std::string configIp(const std::vector<std::string>& args) {
    setupTestableConfigSession("config interface", folly::join(" ", args));
    CmdConfigInterface cmd;
    return cmd.queryClient(localhost(), InterfacesConfig(args));
  }

  std::string deleteIp(const std::vector<std::string>& args) {
    setupTestableConfigSession("delete interface", folly::join(" ", args));
    CmdDeleteInterface cmd;
    return cmd.queryClient(localhost(), InterfaceDeleteConfig(args));
  }
};

// ============================================================================
// Add
// ============================================================================

TEST_F(CmdConfigInterfaceVlanIpTestFixture, addV4ToSvi) {
  auto result = configIp({"vlan2001", "ip-address", "10.0.2.1/24"});
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Contains("10.0.2.1/24"));
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, addV6ToSvi) {
  auto result = configIp({"vlan2001", "ipv6-address", "2001:db8:2::1/64"});
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Contains("2001:db8:2::1/64"));
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, duplicateAddIsIdempotent) {
  auto result = configIp({"vlan2001", "ip-address", "10.0.1.1/24"});
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_EQ(
      std::count(
          sviInterface().ipAddresses()->begin(),
          sviInterface().ipAddresses()->end(),
          "10.0.1.1/24"),
      1);
}

// ============================================================================
// Delete
// ============================================================================

TEST_F(CmdConfigInterfaceVlanIpTestFixture, deleteV4FromSvi) {
  auto result = deleteIp({"vlan2001", "ip-address", "10.0.1.1/24"});
  EXPECT_THAT(result, HasSubstr("Successfully removed"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Not(Contains("10.0.1.1/24")));
  // The other family's address is untouched.
  EXPECT_THAT(*sviInterface().ipAddresses(), Contains("2001:db8:1::1/64"));
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, deleteV6FromSvi) {
  auto result = deleteIp({"vlan2001", "ipv6-address", "2001:db8:1::1/64"});
  EXPECT_THAT(result, HasSubstr("Successfully removed"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Not(Contains("2001:db8:1::1/64")));
}

TEST_F(
    CmdConfigInterfaceVlanIpTestFixture,
    deleteAbsentAddressReportsNoChange) {
  auto result = deleteIp({"vlan2001", "ip-address", "10.9.9.9/24"});
  EXPECT_THAT(result, HasSubstr("not configured"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Contains("10.0.1.1/24"));
}

// ============================================================================
// Refusals
// ============================================================================

TEST_F(CmdConfigInterfaceVlanIpTestFixture, familyMismatchRejected) {
  setupTestableConfigSession();
  CmdConfigInterface cmd;
  EXPECT_THROW(
      cmd.queryClient(
          localhost(),
          InterfacesConfig({"vlan2001", "ip-address", "2001:db8::1/64"})),
      std::invalid_argument);
  EXPECT_THROW(
      cmd.queryClient(
          localhost(),
          InterfacesConfig({"vlan2001", "ipv6-address", "10.0.0.1/24"})),
      std::invalid_argument);
  // Nothing was mutated.
  EXPECT_EQ(sviInterface().ipAddresses()->size(), 2);
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, unknownVlanRejected) {
  setupTestableConfigSession();
  try {
    InterfacesConfig config({"vlan999", "ip-address", "10.0.0.1/24"});
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(std::string(e.what()), HasSubstr("VLAN 999 not found"));
  }
  // No VLAN or interface was created as a side effect.
  const auto& vlans = *swConfig().vlans();
  EXPECT_TRUE(std::none_of(vlans.begin(), vlans.end(), [](const auto& v) {
    return *v.id() == 999;
  }));
  EXPECT_EQ(swConfig().interfaces()->size(), kSeedInterfaces);
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, outOfRangeVlanIdRejected) {
  setupTestableConfigSession();
  for (const auto& name : {"vlan0", "vlan4095", "vlan65537"}) {
    try {
      InterfacesConfig config({name, "ip-address", "10.0.0.1/24"});
      FAIL() << "expected std::invalid_argument for " << name;
    } catch (const std::invalid_argument& e) {
      EXPECT_THAT(std::string(e.what()), HasSubstr("out of range")) << name;
    }
  }
  EXPECT_EQ(swConfig().interfaces()->size(), kSeedInterfaces);
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, vlanWithoutSviRejected) {
  setupTestableConfigSession();
  try {
    InterfacesConfig config({"vlan2002", "ip-address", "10.0.0.1/24"});
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(std::string(e.what()), HasSubstr("no L3 interface"));
  }
  // The interface was not auto-created.
  EXPECT_EQ(swConfig().interfaces()->size(), kSeedInterfaces);
}

// The motivating case: an SVI with no interface name at all is still
// addressable as "vlan<id>", because resolution goes through vlanID.
TEST_F(CmdConfigInterfaceVlanIpTestFixture, unnamedSviResolvedByVlanName) {
  auto result = configIp({"vlan2003", "ip-address", "10.0.4.1/24"});
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(*interfaceById(2003).ipAddresses(), Contains("10.0.4.1/24"));
}

// Two interfaces share vlanID 2004, so "vlan2004" names neither of them.
TEST_F(CmdConfigInterfaceVlanIpTestFixture, ambiguousVlanRejected) {
  setupTestableConfigSession();
  try {
    InterfacesConfig config({"vlan2004", "ip-address", "10.0.5.1/24"});
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(std::string(e.what()), HasSubstr("more than one L3 interface"));
  }
  EXPECT_THAT(*interfaceById(2004).ipAddresses(), IsEmpty());
  EXPECT_THAT(*interfaceById(2005).ipAddresses(), IsEmpty());
}

// Port attributes have nothing to write on a port-less SVI, so the command
// must refuse rather than print success for a no-op.
TEST_F(CmdConfigInterfaceVlanIpTestFixture, portOnlyAttrOnSviRejected) {
  for (const auto& attr : std::vector<std::vector<std::string>>{
           {"vlan2001", "description", "uplink"},
           {"vlan2001", "shutdown"},
           {"vlan2001", "queue-config", "default"}}) {
    try {
      configIp(attr);
      FAIL() << "expected std::invalid_argument for " << attr[1];
    } catch (const std::invalid_argument& e) {
      EXPECT_THAT(std::string(e.what()), HasSubstr("no port")) << attr[1];
    }
  }
}

// ============================================================================
// Existing behavior is preserved
// ============================================================================

TEST_F(CmdConfigInterfaceVlanIpTestFixture, portInterfaceBehaviorUnchanged) {
  configIp({"eth1/1/1", "ip-address", "192.168.1.1/24"});
  const auto& intfs = *swConfig().interfaces();
  auto it = std::find_if(intfs.begin(), intfs.end(), [](const auto& i) {
    return *i.intfID() == 1;
  });
  ASSERT_NE(it, intfs.end());
  EXPECT_THAT(*it->ipAddresses(), Contains("192.168.1.1/24"));
}

// SVI address changes leave the VLAN objects, membership, and unrelated
// interfaces alone.
TEST_F(CmdConfigInterfaceVlanIpTestFixture, noUnrelatedMutation) {
  configIp({"vlan2001", "ip-address", "10.0.3.1/24"});
  for (const auto& v : *swConfig().vlans()) {
    EXPECT_THAT(*v.ipAddresses(), IsEmpty());
  }
  EXPECT_EQ(swConfig().vlanPorts()->size(), 1);
  EXPECT_EQ(*swConfig().vlanPorts()->at(0).vlanID(), 2001);
  const auto& intfs = *swConfig().interfaces();
  auto eth = std::find_if(intfs.begin(), intfs.end(), [](const auto& i) {
    return *i.intfID() == 1;
  });
  ASSERT_NE(eth, intfs.end());
  EXPECT_THAT(*eth->ipAddresses(), IsEmpty());
}

} // namespace facebook::fboss
