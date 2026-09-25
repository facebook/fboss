// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>

#include "fboss/agent/FbossError.h"
#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdDeleteConfigInterfaceTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteConfigInterfaceTestFixture()
      : CmdConfigTestBase(
            "delete_interface_test_%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "interfaces": [
      {
        "intfID": 1,
        "name": "eth1/1/1",
        "vlanID": 100,
        "routerID": 0,
        "ipAddresses": []
      }
    ]
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete interface";
};

// ============================================================================
// InterfaceDeleteConfig Validation Tests
// ============================================================================

// Test valid delete config with single interface
TEST_F(CmdDeleteConfigInterfaceTestFixture, validSingleInterface) {
  setupTestableConfigSession();
  InterfaceDeleteConfig config({"eth1/1/1", "ip-address", "10.0.0.1/24"});
  EXPECT_EQ(config.getInterfaces()[0].name(), "eth1/1/1");
  EXPECT_EQ(config.getAttributes()[0].first, "ip-address");
  EXPECT_EQ(config.getAttributes()[0].second, "10.0.0.1/24");
}

// Test IPv6 address
TEST_F(CmdDeleteConfigInterfaceTestFixture, validIpv6Address) {
  setupTestableConfigSession();
  InterfaceDeleteConfig config({"eth1/1/1", "ipv6-address", "2001:db8::1/64"});
  EXPECT_EQ(config.getInterfaces()[0].name(), "eth1/1/1");
  EXPECT_EQ(config.getAttributes()[0].first, "ipv6-address");
  EXPECT_EQ(config.getAttributes()[0].second, "2001:db8::1/64");
}

// Test case-insensitive attribute names
TEST_F(CmdDeleteConfigInterfaceTestFixture, caseInsensitiveAttribute) {
  setupTestableConfigSession();
  InterfaceDeleteConfig config({"eth1/1/1", "IP-ADDRESS", "10.0.0.1/24"});
  // Attribute should be normalized to lowercase
  EXPECT_EQ(config.getAttributes()[0].first, "ip-address");
}

// Test unknown interface throws (eth1/2/1 is not in the test config)
TEST_F(CmdDeleteConfigInterfaceTestFixture, unknownInterfaceThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig(
          {"eth1/1/1", "eth1/2/1", "ip-address", "10.0.0.1/24"}),
      std::invalid_argument);
}

// Test too few arguments throws
TEST_F(CmdDeleteConfigInterfaceTestFixture, tooFewArgumentsThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig({"eth1/1/1", "ip-address"}), std::invalid_argument);
}

// Test unknown attribute throws
TEST_F(CmdDeleteConfigInterfaceTestFixture, unknownAttributeThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig({"eth1/1/1", "description", "test"}),
      std::invalid_argument);
}

// Test invalid IP address format throws
TEST_F(CmdDeleteConfigInterfaceTestFixture, invalidIpAddressThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig({"eth1/1/1", "ip-address", "invalid"}),
      std::exception);
}

// Test IP version mismatch throws during construction
TEST_F(CmdDeleteConfigInterfaceTestFixture, ipVersionMismatchThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      (InterfaceDeleteConfig({"eth1/1/1", "ip-address", "2001:db8::1/64"})),
      std::invalid_argument);
}

// ============================================================================
// CmdDeleteConfigInterface::queryClient Tests
// ============================================================================

// Test deleting existing IPv4 address
TEST_F(CmdDeleteConfigInterfaceTestFixture, deleteExistingIpv4Address) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ip-address 10.0.0.1/24");

  // First add the IP address
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& intfs = *config.sw()->interfaces();
  for (auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1") {
      intf.ipAddresses()->emplace_back("10.0.0.1/24");
      break;
    }
  }
  session.saveConfig();

  // Now delete it
  auto cmd = CmdDeleteInterface();
  InterfaceDeleteConfig deleteConfig({"eth1/1/1", "ip-address", "10.0.0.1/24"});
  auto result = cmd.queryClient(localhost(), deleteConfig);

  EXPECT_THAT(result, HasSubstr("Successfully removed"));
  EXPECT_THAT(result, HasSubstr("10.0.0.1/24"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  // Verify it was removed
  auto& updatedIntfs = *config.sw()->interfaces();
  for (const auto& intf : updatedIntfs) {
    if (*intf.name() == "eth1/1/1") {
      auto& ips = *intf.ipAddresses();
      EXPECT_EQ(std::find(ips.begin(), ips.end(), "10.0.0.1/24"), ips.end());
    }
  }
}

// Test deleting non-existent IP address (idempotent)
TEST_F(CmdDeleteConfigInterfaceTestFixture, deleteNonExistentIpAddress) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ip-address 10.0.0.1/24");

  auto cmd = CmdDeleteInterface();
  InterfaceDeleteConfig deleteConfig({"eth1/1/1", "ip-address", "10.0.0.1/24"});
  auto result = cmd.queryClient(localhost(), deleteConfig);

  EXPECT_THAT(result, HasSubstr("not configured"));
}

// ============================================================================
// Whole-port delete (bare `delete interface <port>`, no attributes)
// ============================================================================

// Fixture with a full port -> vlanPort -> vlan -> interface chain so we can
// verify that deleting a port also prunes its dependents.
class CmdDeleteWholeInterfaceTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteWholeInterfaceTestFixture()
      : CmdConfigTestBase(
            "delete_whole_interface_test_%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 100
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 200
      }
    ],
    "vlanPorts": [
      {"vlanID": 100, "logicalPort": 1, "spanningTreeState": 2, "emitTags": false},
      {"vlanID": 200, "logicalPort": 2, "spanningTreeState": 2, "emitTags": false}
    ],
    "defaultVlan": 4000,
    "vlans": [
      {"id": 100, "name": "vlan100", "routable": true, "intfID": 100},
      {"id": 200, "name": "vlan200", "routable": true, "intfID": 200},
      {"id": 4000, "name": "vlan4000", "routable": false, "intfID": 0}
    ],
    "interfaces": [
      {"intfID": 100, "vlanID": 100, "routerID": 0, "type": 1, "mtu": 9412},
      {"intfID": 200, "vlanID": 200, "routerID": 0, "type": 1, "mtu": 9412}
    ]
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete interface";
};

// Bare `delete interface eth1/1/1` removes the port and its vlanPort, the now
// empty vlan, and the vlan's interface, while leaving the other port's chain.
TEST_F(CmdDeleteWholeInterfaceTestFixture, deletesPortAndDependents) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1");
  auto cmd = CmdDeleteInterface();
  InterfaceDeleteConfig deleteConfig({"eth1/1/1"});

  auto result = cmd.queryClient(localhost(), deleteConfig);

  EXPECT_THAT(result, HasSubstr("Deleted interface(s)"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();

  const auto& ports = *swConfig.ports();
  EXPECT_EQ(
      std::none_of(
          ports.begin(),
          ports.end(),
          [](const auto& p) { return *p.logicalID() == 1; }),
      true);

  const auto& vlanPorts = *swConfig.vlanPorts();
  EXPECT_EQ(
      std::none_of(
          vlanPorts.begin(),
          vlanPorts.end(),
          [](const auto& vp) { return *vp.logicalPort() == 1; }),
      true);

  const auto& vlans = *swConfig.vlans();
  EXPECT_EQ(
      std::none_of(
          vlans.begin(),
          vlans.end(),
          [](const auto& v) { return *v.id() == 100; }),
      true);

  const auto& interfaces = *swConfig.interfaces();
  EXPECT_EQ(
      std::none_of(
          interfaces.begin(),
          interfaces.end(),
          [](const auto& i) { return *i.intfID() == 100; }),
      true);

  // The other port's chain is untouched.
  EXPECT_EQ(
      std::any_of(
          ports.begin(),
          ports.end(),
          [](const auto& p) { return *p.logicalID() == 2; }),
      true);
  EXPECT_EQ(
      std::any_of(
          vlans.begin(),
          vlans.end(),
          [](const auto& v) { return *v.id() == 200; }),
      true);
  EXPECT_EQ(
      std::any_of(
          interfaces.begin(),
          interfaces.end(),
          [](const auto& i) { return *i.intfID() == 200; }),
      true);
}

// ============================================================================
// queue-config reset
// ============================================================================

// `delete interface <intf> queue-config` is the same reset as
// `config interface <intf> queue-config default`: it clears
// Port::portQueueConfigName, which makes the port fall back to
// SwitchConfig::defaultPortQueues.
class DeleteQueueConfigAttrTestFixture : public CmdConfigTestBase {
 public:
  DeleteQueueConfigAttrTestFixture()
      : CmdConfigTestBase(
            "delete_ifqc_attr_test_%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1,
        "portQueueConfigName": "rsw_queues"
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1
      }
    ],
    "portQueueConfigs": {
      "rsw_queues": [
        {"id": 0, "streamType": 1, "weight": 1, "scheduling": 5}
      ]
    }
  }
})") {}

 protected:
  static const cfg::Port* findPort(const std::string& name) {
    const auto& ports =
        *ConfigSession::getInstance().getAgentConfig().sw()->ports();
    for (const auto& port : ports) {
      if (*port.name() == name) {
        return &port;
      }
    }
    return nullptr;
  }

  std::string runDelete(const std::vector<std::string>& args) {
    auto cmd = CmdDeleteInterface();
    return cmd.queryClient(localhost(), InterfaceDeleteConfig(args));
  }
};

// queue-config takes no value on the delete side -- it is a reset, not a
// removal of a particular binding.
TEST_F(DeleteQueueConfigAttrTestFixture, parsesAsValuelessAttribute) {
  setupTestableConfigSession();
  InterfaceDeleteConfig config({"eth1/1/1", "queue-config"});
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "queue-config");
  EXPECT_EQ(config.getAttributes()[0].second, "");
}

TEST_F(DeleteQueueConfigAttrTestFixture, clearsExistingBinding) {
  setupTestableConfigSession();
  ASSERT_TRUE(findPort("eth1/1/1")->portQueueConfigName().has_value());

  auto result = runDelete({"eth1/1/1", "queue-config"});
  EXPECT_THAT(result, ::testing::HasSubstr("queue-config"));

  EXPECT_FALSE(findPort("eth1/1/1")->portQueueConfigName().has_value());
  // Clearing a binding must not remove the config it pointed at.
  EXPECT_TRUE(
      ConfigSession::getInstance()
          .getAgentConfig()
          .sw()
          ->portQueueConfigs()
          ->count("rsw_queues"));
}

TEST_F(DeleteQueueConfigAttrTestFixture, unboundPortIsNoOp) {
  setupTestableConfigSession();
  EXPECT_NO_THROW(runDelete({"eth1/2/1", "queue-config"}));
  EXPECT_FALSE(findPort("eth1/2/1")->portQueueConfigName().has_value());
}

// ============================================================================
// Whole-interface delete (bare `delete interface <intfID|name>`)
// ============================================================================

// Seed mirrors the shape a generated agent.conf has: interfaces carry no name
// (so the interface ID is the only handle). Numeric enum values are
// cfg::PortState (1 DISABLED, 2 ENABLED) and cfg::InterfaceType (1 VLAN,
// 3 PORT).
//
// Interfaces, and what each is here to exercise:
//   10   virtual loopback, VLAN 10 has no member ports     -> deleted, VLAN
//                                                             cascaded
//   100  VLAN 100 has an enabled member port               -> refused
//   200  VLAN 200's only member port is disabled but       -> refused (ingress
//        names it as its ingress VLAN                         VLAN)
//   600  VLAN 600's only member port is disabled and       -> deleted, VLAN +
//        tagged (its ingress VLAN is elsewhere)               membership row
//                                                             cascaded
//   4094 the default VLAN's interface                      -> deleted, VLAN
//                                                             kept (default)
//   2001 port router interface for port 1                  -> refused
class CmdDeleteWholeL3InterfaceTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteWholeL3InterfaceTestFixture()
      : CmdConfigTestBase(
            "delete_l3_interface_test_%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [
      {"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000, "ingressVlan": 100},
      {"logicalID": 2, "name": "eth1/2/1", "state": 1, "speed": 100000, "ingressVlan": 200},
      {"logicalID": 6, "name": "eth1/6/1", "state": 1, "speed": 100000, "ingressVlan": 4094}
    ],
    "vlanPorts": [
      {"vlanID": 100, "logicalPort": 1, "spanningTreeState": 2, "emitTags": false},
      {"vlanID": 200, "logicalPort": 2, "spanningTreeState": 2, "emitTags": false},
      {"vlanID": 600, "logicalPort": 6, "spanningTreeState": 2, "emitTags": false}
    ],
    "defaultVlan": 4094,
    "vlans": [
      {"id": 10, "name": "fbossLoopback0", "routable": true, "intfID": 10},
      {"id": 100, "name": "vlan100", "routable": true, "intfID": 100},
      {"id": 200, "name": "vlan200", "routable": true, "intfID": 200},
      {"id": 600, "name": "vlan600", "routable": true, "intfID": 600},
      {"id": 4094, "name": "default", "routable": false, "intfID": 4094}
    ],
    "interfaces": [
      {"intfID": 10, "vlanID": 10, "routerID": 0, "type": 1, "isVirtual": true,
       "ipAddresses": ["10.0.0.1/32"]},
      {"intfID": 100, "vlanID": 100, "routerID": 0, "type": 1, "mtu": 9412},
      {"intfID": 200, "vlanID": 200, "routerID": 0, "type": 1, "mtu": 9412},
      {"intfID": 600, "vlanID": 600, "routerID": 0, "type": 1, "mtu": 9412},
      {"intfID": 4094, "vlanID": 4094, "routerID": 0, "type": 1, "mtu": 9412},
      {"intfID": 2001, "vlanID": 0, "routerID": 0, "type": 3, "portID": 1}
    ]
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete interface";

  static cfg::SwitchConfig& swConfig() {
    return *ConfigSession::getInstance().getAgentConfig().sw();
  }

  static bool hasInterface(int32_t intfId) {
    const auto& interfaces = *swConfig().interfaces();
    return std::any_of(
        interfaces.begin(), interfaces.end(), [intfId](const auto& intf) {
          return *intf.intfID() == intfId;
        });
  }

  static bool hasPort(int32_t logicalId) {
    const auto& ports = *swConfig().ports();
    return std::any_of(ports.begin(), ports.end(), [logicalId](const auto& p) {
      return *p.logicalID() == logicalId;
    });
  }

  static bool hasVlan(int32_t vlanId) {
    const auto& vlans = *swConfig().vlans();
    return std::any_of(vlans.begin(), vlans.end(), [vlanId](const auto& v) {
      return *v.id() == vlanId;
    });
  }

  // Runs `delete interface <names...>` through the real arg type and handler.
  std::string runDelete(const std::vector<std::string>& names) {
    auto cmd = CmdDeleteInterface();
    return cmd.queryClient(localhost(), InterfaceDeleteConfig(names));
  }
};

// A bare interface ID resolves even though the interface has no name, and the
// virtual loopback interface deletes: its VLAN has no member ports at all.
TEST_F(CmdDeleteWholeL3InterfaceTestFixture, deletesLoopbackInterfaceById) {
  setupTestableConfigSession(cmdPrefix_, "10");

  auto result = runDelete({"10"});

  EXPECT_THAT(result, HasSubstr("Deleted interface(s)"));
  EXPECT_THAT(result, HasSubstr("10"));
  EXPECT_FALSE(hasInterface(10));

  // The VLAN was left without an interface and nothing else references it, so
  // it is cascaded along with the delete.
  EXPECT_FALSE(hasVlan(10));
  EXPECT_THAT(result, HasSubstr("also removed VLAN(s) 10"));
}

// The only interface for a VLAN that still has an enabled member port is
// refused: ThriftConfigApplier would reject the resulting config.
TEST_F(CmdDeleteWholeL3InterfaceTestFixture, refusesWhenVlanHasEnabledPort) {
  setupTestableConfigSession(cmdPrefix_, "100");

  try {
    runDelete({"100"});
    FAIL() << "expected the delete to be refused";
  } catch (const FbossError& e) {
    EXPECT_THAT(std::string(e.what()), HasSubstr("enabled member port"));
    EXPECT_THAT(std::string(e.what()), HasSubstr("eth1/1/1"));
  }
  EXPECT_TRUE(hasInterface(100));
}

// The member port is disabled, but it still uses VLAN 200 as its ingress VLAN,
// so the VLAN cannot be removed with its interface.
TEST_F(
    CmdDeleteWholeL3InterfaceTestFixture,
    refusesWhenDisabledPortUsesVlanAsIngress) {
  setupTestableConfigSession(cmdPrefix_, "200");

  try {
    runDelete({"200"});
    FAIL() << "expected the delete to be refused";
  } catch (const FbossError& e) {
    EXPECT_THAT(std::string(e.what()), HasSubstr("ingress VLAN"));
    EXPECT_THAT(std::string(e.what()), HasSubstr("eth1/2/1"));
  }
  EXPECT_TRUE(hasInterface(200));
  EXPECT_TRUE(hasVlan(200));
}

// A disabled port that is only a tagged member of the VLAN (its ingress VLAN
// is elsewhere) does not block the delete: the VLAN and its membership row
// are cascaded, the port itself survives.
TEST_F(CmdDeleteWholeL3InterfaceTestFixture, cascadesVlanWithTaggedMemberOnly) {
  setupTestableConfigSession(cmdPrefix_, "600");

  auto result = runDelete({"600"});

  EXPECT_THAT(result, HasSubstr("also removed VLAN(s) 600"));
  EXPECT_FALSE(hasInterface(600));
  EXPECT_FALSE(hasVlan(600));
  EXPECT_TRUE(hasPort(6));
  const auto& vlanPorts = *swConfig().vlanPorts();
  EXPECT_TRUE(
      std::none_of(vlanPorts.begin(), vlanPorts.end(), [](const auto& vp) {
        return *vp.vlanID() == 600;
      }));
}

// Deleting the default VLAN's interface keeps the VLAN and clears its intfID.
TEST_F(CmdDeleteWholeL3InterfaceTestFixture, keepsDefaultVlanWithoutCascade) {
  setupTestableConfigSession(cmdPrefix_, "4094");

  auto result = runDelete({"4094"});

  EXPECT_THAT(result, HasSubstr("Deleted interface(s)"));
  EXPECT_THAT(result, Not(HasSubstr("also removed VLAN")));
  EXPECT_FALSE(hasInterface(4094));
  const auto& vlans = *swConfig().vlans();
  auto vlan = std::find_if(vlans.begin(), vlans.end(), [](const auto& v) {
    return *v.id() == 4094;
  });
  ASSERT_NE(vlan, vlans.end());
  EXPECT_FALSE(vlan->intfID().has_value());
}

// A port router interface alone is refused: the agent CHECK-fails on a port
// with no interface.
TEST_F(CmdDeleteWholeL3InterfaceTestFixture, refusesPortRouterInterface) {
  setupTestableConfigSession(cmdPrefix_, "2001");

  try {
    runDelete({"2001"});
    FAIL() << "expected the delete to be refused";
  } catch (const FbossError& e) {
    EXPECT_THAT(std::string(e.what()), HasSubstr("port router interface"));
  }
  EXPECT_TRUE(hasInterface(2001));
}

// A refusal anywhere in a multi-interface delete leaves every interface in
// place, rather than applying the ones checked before the failure.
TEST_F(CmdDeleteWholeL3InterfaceTestFixture, refusalLeavesConfigUntouched) {
  setupTestableConfigSession(cmdPrefix_, "10 100");

  EXPECT_THROW(runDelete({"10", "100"}), FbossError);
  EXPECT_TRUE(hasInterface(10)) << "deletable interface must not be applied";
  EXPECT_TRUE(hasInterface(100));
}

// Naming an SVI and its VLAN's only (enabled) member port in one command
// deletes both: the port being deleted does not block the SVI delete.
TEST_F(
    CmdDeleteWholeL3InterfaceTestFixture,
    deletesInterfaceAndItsPortTogether) {
  setupTestableConfigSession(cmdPrefix_, "100 eth1/1/1");

  EXPECT_THAT(
      runDelete({"100", "eth1/1/1"}), HasSubstr("Deleted interface(s)"));

  EXPECT_FALSE(hasInterface(100)) << "the SVI is deleted";
  EXPECT_FALSE(hasPort(1)) << "its member port is deleted in the same command";
  EXPECT_FALSE(hasVlan(100))
      << "the port being deleted does not block the VLAN cascade";
}

} // namespace facebook::fboss
