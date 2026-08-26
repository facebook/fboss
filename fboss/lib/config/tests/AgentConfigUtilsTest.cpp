/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/config/AgentConfigUtils.h"

namespace facebook::fboss {

namespace {
constexpr auto kProfile = cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91;
constexpr auto kProfileNoConfig =
    cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N;
constexpr auto kProfileSpeed = cfg::PortSpeed::HUNDREDG;
const PortID kPortId{1};
constexpr auto kPortName = "eth1/1/1";

cfg::PlatformPortEntry makeEntry(int32_t id, const std::string& name) {
  cfg::PlatformPortEntry entry;
  entry.mapping()->id() = id;
  entry.mapping()->name() = name;
  entry.mapping()->controllingPort() = id;
  entry.mapping()->portType() = cfg::PortType::INTERFACE_PORT;
  entry.mapping()->scope() = cfg::Scope::LOCAL;
  entry.supportedProfiles()[kProfile] = cfg::PlatformPortConfig{};
  entry.supportedProfiles()[kProfileNoConfig] = cfg::PlatformPortConfig{};
  return entry;
}

// Build a PlatformMapping with a single port. Only kProfile has a resolvable
// PortProfileConfig (speed); kProfileNoConfig deliberately has none.
PlatformMapping makeMapping() {
  cfg::PlatformMapping thriftMapping;
  thriftMapping.ports()[kPortId] = makeEntry(kPortId, kPortName);

  cfg::PlatformPortProfileConfigEntry profileEntry;
  profileEntry.factor()->profileID() = kProfile;
  profileEntry.profile()->speed() = kProfileSpeed;
  thriftMapping.platformSupportedProfiles()->push_back(profileEntry);

  return PlatformMapping(thriftMapping);
}
} // namespace

TEST(AgentConfigUtilsTest, createDefaultPortConfigSetsAllFields) {
  auto mapping = makeMapping();

  auto port =
      utility::createDefaultPortConfig(&mapping, kPortId, kProfile, 2001);

  EXPECT_EQ(*port.name(), kPortName);
  EXPECT_EQ(PortID(*port.logicalID()), kPortId);
  EXPECT_EQ(*port.profileID(), kProfile);
  EXPECT_EQ(*port.portType(), cfg::PortType::INTERFACE_PORT);
  EXPECT_EQ(*port.scope(), cfg::Scope::LOCAL);
  EXPECT_EQ(*port.state(), cfg::PortState::DISABLED);
  EXPECT_EQ(*port.ingressVlan(), 2001);
  EXPECT_EQ(*port.speed(), kProfileSpeed);
}

TEST(AgentConfigUtilsTest, createDefaultPortConfigDefaultsSpeedWhenUnresolved) {
  auto mapping = makeMapping();

  auto port = utility::createDefaultPortConfig(
      &mapping, kPortId, kProfileNoConfig, 2001);

  EXPECT_EQ(*port.speed(), cfg::PortSpeed::DEFAULT);
}

TEST(AgentConfigUtilsTest, allocateFreeVlanIdSkipsVlanAndInterfaceIds) {
  cfg::SwitchConfig config;
  cfg::Vlan vlan2001;
  vlan2001.id() = 2001;
  cfg::Vlan vlan2003;
  vlan2003.id() = 2003;
  config.vlans() = {vlan2001, vlan2003};
  cfg::Interface intf;
  intf.intfID() = 2002;
  config.interfaces() = {intf};

  EXPECT_EQ(utility::allocateFreeVlanId(config), 2004);
}

TEST(AgentConfigUtilsTest, allocateFreeVlanIdReturnsNextAfterSingleVlan) {
  cfg::SwitchConfig config;
  cfg::Vlan vlan2001;
  vlan2001.id() = 2001;
  config.vlans() = {vlan2001};

  EXPECT_EQ(utility::allocateFreeVlanId(config), 2002);
}

TEST(AgentConfigUtilsTest, allocateFreeVlanIdThrowsWhenExhausted) {
  cfg::SwitchConfig config;
  cfg::Vlan vlan;
  vlan.id() = 2001;
  config.vlans() = {vlan};

  EXPECT_THROW(utility::allocateFreeVlanId(config, 2001, 2001), FbossError);
}

TEST(AgentConfigUtilsTest, addInterfacePortToConfigAppendsAllEntities) {
  auto mapping = makeMapping();
  cfg::SwitchConfig config;
  // Occupy 2001 so the allocation must skip it.
  cfg::Vlan existing;
  existing.id() = 2001;
  config.vlans() = {existing};

  const int32_t n =
      utility::addInterfacePortToConfig(config, &mapping, kPortId, kProfile);

  EXPECT_EQ(n, 2002);

  ASSERT_EQ(config.ports()->size(), 1);
  const auto& port = config.ports()->at(0);
  EXPECT_TRUE(*port.routable());
  EXPECT_EQ(*port.ingressVlan(), n);
  EXPECT_EQ(*port.state(), cfg::PortState::DISABLED);
  EXPECT_EQ(PortID(*port.logicalID()), kPortId);

  ASSERT_EQ(config.vlans()->size(), 2);
  const auto& vlan = config.vlans()->at(1);
  EXPECT_EQ(*vlan.id(), n);
  EXPECT_TRUE(*vlan.routable());
  EXPECT_TRUE(*vlan.recordStats());

  ASSERT_EQ(config.vlanPorts()->size(), 1);
  const auto& vlanPort = config.vlanPorts()->at(0);
  EXPECT_EQ(*vlanPort.vlanID(), n);
  EXPECT_EQ(*vlanPort.logicalPort(), static_cast<int32_t>(kPortId));
  EXPECT_EQ(*vlanPort.spanningTreeState(), cfg::SpanningTreeState::FORWARDING);
  EXPECT_FALSE(*vlanPort.emitTags());

  ASSERT_EQ(config.interfaces()->size(), 1);
  const auto& intf = config.interfaces()->at(0);
  EXPECT_EQ(*intf.intfID(), n);
  EXPECT_EQ(*intf.vlanID(), n);
  EXPECT_EQ(*intf.type(), cfg::InterfaceType::VLAN);
  EXPECT_EQ(*intf.routerID(), 0);
  EXPECT_EQ(*intf.scope(), cfg::Scope::LOCAL);
  EXPECT_FALSE(intf.mac().has_value());
  EXPECT_TRUE(intf.ipAddresses()->empty());
}

namespace {
cfg::Port makePort(int id, int ingressVlan) {
  cfg::Port p;
  p.logicalID() = id;
  p.name() = std::string("eth1/") + std::to_string(id) + "/1";
  p.ingressVlan() = ingressVlan;
  return p;
}
cfg::Vlan makeVlan(int id) {
  cfg::Vlan v;
  v.id() = id;
  v.name() = std::string("vlan") + std::to_string(id);
  return v;
}
cfg::VlanPort makeVlanPort(int logicalPort, int vlan) {
  cfg::VlanPort vp;
  vp.logicalPort() = logicalPort;
  vp.vlanID() = vlan;
  return vp;
}
cfg::Interface makeVlanIntf(int intfId, int vlan) {
  cfg::Interface i;
  i.intfID() = intfId;
  i.vlanID() = vlan;
  return i;
}
} // namespace

// Removing a subsumed port drops the port, its vlanPort, the vlan it emptied,
// and the VLAN interface on that vlan; while keeping a surviving port's vlan,
// an originally-empty vlan, and the default vlan.
TEST(RemovePortsFromConfigTest, prunesPortVlanPortVlanAndInterface) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  config.ports() = {makePort(1, 1), makePort(2, 2)}; // port 2 will be subsumed
  config.vlans() = {makeVlan(1), makeVlan(2), makeVlan(99)}; // 99 is empty
  config.vlanPorts() = {makeVlanPort(1, 1), makeVlanPort(2, 2)};
  config.interfaces() = {makeVlanIntf(1, 1), makeVlanIntf(2, 2)};

  utility::removePortsFromConfig(config, {PortID(2)});

  cfg::SwitchConfig expected;
  expected.defaultVlan() = 1;
  expected.ports() = {makePort(1, 1)};
  expected.vlans() = {makeVlan(1), makeVlan(99)};
  expected.vlanPorts() = {makeVlanPort(1, 1)};
  expected.interfaces() = {makeVlanIntf(1, 1)};

  EXPECT_EQ(config, expected);
}

// Vlan membership is judged by vlanPorts, not by ports' ingressVlan: a vlan
// owned as ingressVlan by a removed port survives if a surviving port is still
// a (tagged) member of it via a vlanPort -- so no vlanPort is left pointing at
// a dropped vlan.
TEST(RemovePortsFromConfigTest, keepsVlanReferencedBySurvivingTaggedPort) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  config.ports() = {makePort(1, 1), makePort(2, 2)}; // port 2 removed
  config.vlans() = {makeVlan(1), makeVlan(2)};
  // port 1 is untagged on vlan 1 and a tagged member of vlan 2 (port 2's
  // ingressVlan).
  config.vlanPorts() = {
      makeVlanPort(1, 1), makeVlanPort(1, 2), makeVlanPort(2, 2)};
  config.interfaces() = {makeVlanIntf(1, 1), makeVlanIntf(2, 2)};

  utility::removePortsFromConfig(config, {PortID(2)});

  // vlan 2 (and its interface) survive because port 1 still references it via a
  // vlanPort; only port 2's own vlanPort is dropped -- nothing dangles.
  cfg::SwitchConfig expected;
  expected.defaultVlan() = 1;
  expected.ports() = {makePort(1, 1)};
  expected.vlans() = {makeVlan(1), makeVlan(2)};
  expected.vlanPorts() = {makeVlanPort(1, 1), makeVlanPort(1, 2)};
  expected.interfaces() = {makeVlanIntf(1, 1), makeVlanIntf(2, 2)};

  EXPECT_EQ(config, expected);
}

// The default vlan is preserved even when the port that used it is removed and
// no other vlanPort references it.
TEST(RemovePortsFromConfigTest, keepsDefaultVlanEvenWhenEmptied) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  config.ports() = {makePort(1, 1), makePort(2, 2)}; // port 1 uses default vlan
  config.vlans() = {makeVlan(1), makeVlan(2)};
  config.vlanPorts() = {makeVlanPort(1, 1), makeVlanPort(2, 2)};
  config.interfaces() = {makeVlanIntf(1, 1), makeVlanIntf(2, 2)};

  utility::removePortsFromConfig(config, {PortID(1)});

  // Default vlan 1 survives despite losing its only vlanPort; vlan 2 (still
  // used by surviving port 2) also survives.
  cfg::SwitchConfig expected;
  expected.defaultVlan() = 1;
  expected.ports() = {makePort(2, 2)};
  expected.vlans() = {makeVlan(1), makeVlan(2)};
  expected.vlanPorts() = {makeVlanPort(2, 2)};
  expected.interfaces() = {makeVlanIntf(1, 1), makeVlanIntf(2, 2)};

  EXPECT_EQ(config, expected);
}

// VLAN-only for now: port-router (PORT-type) interfaces bind to a port via
// portID, not a vlan (their vlanID defaults to 0), so removePortsFromConfig
// leaves them untouched -- both on a surviving port and (for now) on the
// removed port. Full port-router cleanup is deferred until platform support is
// verified. This guards against the vlan-survival check erroneously erasing an
// L3 interface whose vlanID reads as 0.
TEST(RemovePortsFromConfigTest, leavesPortRouterInterfacesUntouched) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  config.ports() = {makePort(1, 1), makePort(2, 2)}; // port 2 removed
  config.vlans() = {makeVlan(1), makeVlan(2)};
  config.vlanPorts() = {makeVlanPort(1, 1), makeVlanPort(2, 2)};
  cfg::Interface portIntf1; // port-router interface on surviving port 1
  portIntf1.intfID() = 101;
  portIntf1.portID() = 1;
  cfg::Interface portIntf2; // port-router interface on removed port 2
  portIntf2.intfID() = 102;
  portIntf2.portID() = 2;
  config.interfaces() = {portIntf1, portIntf2};

  utility::removePortsFromConfig(config, {PortID(2)});

  // Both port-router interfaces remain (not pruned by the vlan-survival check).
  ASSERT_EQ(config.interfaces()->size(), 2);
  EXPECT_EQ(*config.interfaces()->at(0).intfID(), 101);
  EXPECT_EQ(*config.interfaces()->at(1).intfID(), 102);
}

// An empty subsumed set leaves the config untouched.
TEST(RemovePortsFromConfigTest, emptySubsumedSetIsNoOp) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  config.ports() = {makePort(1, 1)};
  config.vlans() = {makeVlan(1)};
  config.vlanPorts() = {makeVlanPort(1, 1)};
  config.interfaces() = {makeVlanIntf(1, 1)};

  const cfg::SwitchConfig expected = config;
  utility::removePortsFromConfig(config, {});
  EXPECT_EQ(config, expected);
}

// Disable mode keeps the subsumed port (and its vlan/vlanPort/interface) in
// place but marks the port DISABLED.
TEST(RemovePortsFromConfigTest, disableModeDisablesPortInPlace) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  config.ports() = {makePort(1, 1), makePort(2, 2)};
  config.vlans() = {makeVlan(1), makeVlan(2)};
  config.vlanPorts() = {makeVlanPort(1, 1), makeVlanPort(2, 2)};
  config.interfaces() = {makeVlanIntf(1, 1), makeVlanIntf(2, 2)};

  utility::removePortsFromConfig(
      config, {PortID(2)}, utility::PortRemovalMode::Disable);

  ASSERT_EQ(config.ports()->size(), 2);
  for (const auto& p : *config.ports()) {
    if (*p.logicalID() == 2) {
      EXPECT_EQ(*p.state(), cfg::PortState::DISABLED);
    }
  }
  EXPECT_EQ(config.vlanPorts()->size(), 2);
  EXPECT_EQ(config.vlans()->size(), 2);
  EXPECT_EQ(config.interfaces()->size(), 2);
}

// Erase mode with pruning disabled drops the port and its vlanPort but leaves
// the now-empty vlan and the interface untouched.
TEST(RemovePortsFromConfigTest, eraseWithoutPruneKeepsVlansAndIntfs) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  config.ports() = {makePort(1, 1), makePort(2, 2)};
  config.vlans() = {makeVlan(1), makeVlan(2)};
  config.vlanPorts() = {makeVlanPort(1, 1), makeVlanPort(2, 2)};
  config.interfaces() = {makeVlanIntf(1, 1), makeVlanIntf(2, 2)};

  utility::removePortsFromConfig(
      config,
      {PortID(2)},
      utility::PortRemovalMode::Erase,
      /*pruneEmptyVlansAndInterfaces=*/false);

  ASSERT_EQ(config.ports()->size(), 1);
  EXPECT_EQ(*config.ports()->at(0).logicalID(), 1);
  EXPECT_EQ(config.vlanPorts()->size(), 1);
  EXPECT_EQ(config.vlans()->size(), 2);
  EXPECT_EQ(config.interfaces()->size(), 2);
}

} // namespace facebook::fboss
