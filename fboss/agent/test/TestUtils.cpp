/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/TestUtils.h"

#include <boost/cast.hpp>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/MockTunManager.h"

#include "fboss/agent/rib/RoutingInformationBase.h"

#include <folly/Memory.h>
#include <folly/json.h>
#include <chrono>
#include <optional>

using folly::ByteRange;
using folly::IOBuf;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::StringPiece;
using folly::io::Cursor;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using namespace facebook::fboss;

namespace {

void initSwSwitchWithFlags(SwSwitch* sw, SwitchFlags flags) {
  if (flags & SwitchFlags::ENABLE_TUN) {
    // TODO(aeckert): I don't think this should be a first class
    // argument to SwSwitch::init() as unit tests are the only place
    // that pass in a TunManager to init(). Let's come up with a way
    // to mock the TunManager initialization instead of passing it in
    // like this.
    //
    // TODO(aeckert): Have MockTunManager hit the real TunManager
    // implementation if testing on actual hw
    auto mockTunMgr =
        std::make_unique<MockTunManager>(sw, sw->getBackgroundEvb());
    EXPECT_CALL(*mockTunMgr.get(), doProbe(_)).Times(1);
    sw->init(std::move(mockTunMgr), flags);
  } else {
    sw->init(nullptr, flags);
  }
}

unique_ptr<SwSwitch> createMockSw(
    const shared_ptr<SwitchState>& state,
    const std::optional<MacAddress>& mac,
    SwitchFlags flags) {
  auto platform = createMockPlatform();
  if (mac) {
    EXPECT_CALL(*platform.get(), getLocalMac())
        .WillRepeatedly(Return(mac.value()));
  }
  return setupMockSwitchWithoutHW(std::move(platform), state, flags);
}

shared_ptr<SwitchState> setAllPortState(
    const shared_ptr<SwitchState>& in,
    bool up) {
  auto newState = in->clone();
  auto newPortMap = newState->getPorts()->modify(&newState);
  for (auto port : *newPortMap) {
    auto newPort = port->clone();
    newPort->setOperState(up);
    newPort->setAdminState(
        up ? cfg::PortState::ENABLED : cfg::PortState::DISABLED);
    newPortMap->updatePort(newPort);
  }
  return newState;
}
} // namespace

namespace facebook::fboss {

shared_ptr<SwitchState> bringAllPortsUp(const shared_ptr<SwitchState>& in) {
  return setAllPortState(in, true);
}
shared_ptr<SwitchState> bringAllPortsDown(const shared_ptr<SwitchState>& in) {
  return setAllPortState(in, false);
}

shared_ptr<SwitchState> publishAndApplyConfig(
    shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    rib::RoutingInformationBase* rib) {
  state->publish();
  return applyThriftConfig(state, config, platform, rib);
}

std::unique_ptr<SwSwitch> setupMockSwitchWithoutHW(
    std::unique_ptr<MockPlatform> platform,
    const std::shared_ptr<SwitchState>& state,
    SwitchFlags flags) {
  auto sw = make_unique<SwSwitch>(std::move(platform));
  HwInitResult ret;
  ret.switchState = state ? state : make_shared<SwitchState>();
  ret.bootType = BootType::COLD_BOOT;
  EXPECT_HW_CALL(sw, init(_)).WillOnce(Return(ret));
  initSwSwitchWithFlags(sw.get(), flags);
  waitForStateUpdates(sw.get());
  return sw;
}

unique_ptr<MockPlatform> createMockPlatform() {
  return make_unique<testing::NiceMock<MockPlatform>>();
}

unique_ptr<HwTestHandle> createTestHandle(
    const shared_ptr<SwitchState>& state,
    const std::optional<MacAddress>& mac,
    SwitchFlags flags) {
  auto sw = createMockSw(state, mac, flags);
  auto platform = static_cast<MockPlatform*>(sw->getPlatform());
  auto handle = platform->createTestHandle(std::move(sw));
  handle->prepareForTesting();
  return handle;
}

unique_ptr<HwTestHandle>
createTestHandle(cfg::SwitchConfig* config, MacAddress mac, SwitchFlags flags) {
  shared_ptr<SwitchState> initialState{nullptr};
  // Create the initial state, which only has ports
  initialState = make_shared<SwitchState>();
  uint32_t maxPort{0};
  for (const auto& port : *config->ports_ref()) {
    maxPort = std::max(static_cast<int32_t>(maxPort), *port.logicalID_ref());
  }
  for (uint32_t idx = 1; idx <= maxPort; ++idx) {
    initialState->registerPort(PortID(idx), folly::to<string>("port", idx));
  }

  auto handle = createTestHandle(initialState, mac, flags);
  auto sw = handle->getSw();

  // Apply the thrift config
  auto updateFn = [&](const shared_ptr<SwitchState>& state) {
    return applyThriftConfig(
        state,
        config,
        sw->getPlatform(),
        sw->isStandaloneRibEnabled() ? sw->getRib() : nullptr);
  };
  sw->updateStateBlocking("test_setup", updateFn);
  return handle;
}

unique_ptr<HwTestHandle> createTestHandle(
    cfg::SwitchConfig* config,
    SwitchFlags flags) {
  return createTestHandle(config, MacAddress("02:00:00:00:00:01"), flags);
}

MockHwSwitch* getMockHw(SwSwitch* sw) {
  return boost::polymorphic_downcast<MockHwSwitch*>(sw->getHw());
}

MockPlatform* getMockPlatform(SwSwitch* sw) {
  return boost::polymorphic_downcast<MockPlatform*>(sw->getPlatform());
}

MockHwSwitch* getMockHw(std::unique_ptr<SwSwitch>& sw) {
  return boost::polymorphic_downcast<MockHwSwitch*>(sw->getHw());
}

MockPlatform* getMockPlatform(std::unique_ptr<SwSwitch>& sw) {
  return boost::polymorphic_downcast<MockPlatform*>(sw->getPlatform());
}

std::shared_ptr<SwitchState> waitForStateUpdates(SwSwitch* sw) {
  // All StateUpdates scheduled from this thread will be applied in order,
  // so we can simply perform a blocking no-op update.  When it is done
  // we can be sure that all previously scheduled updates have also been
  // applied.
  std::shared_ptr<SwitchState> snapshot{nullptr};
  auto snapshotUpdate = [&snapshot](const shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    // take a snapshot of the current state, but don't modify state
    snapshot = state;
    return nullptr;
  };
  sw->updateStateBlocking("waitForStateUpdates", snapshotUpdate);
  return snapshot;
}

void waitForBackgroundThread(SwSwitch* sw) {
  auto* evb = sw->getBackgroundEvb();
  evb->runInEventBaseThreadAndWait([]() { return; });
}

cfg::SwitchConfig testConfigA() {
  cfg::SwitchConfig cfg;
  static constexpr auto kPortCount = 20;

  cfg.ports_ref()->resize(kPortCount);
  for (int p = 0; p < kPortCount; ++p) {
    *cfg.ports_ref()[p].logicalID_ref() = p + 1;
    cfg.ports_ref()[p].name_ref() = folly::to<string>("port", p + 1);
  }

  cfg.vlans_ref()->resize(2);
  *cfg.vlans_ref()[0].id_ref() = 1;
  *cfg.vlans_ref()[0].name_ref() = "Vlan1";
  cfg.vlans_ref()[0].intfID_ref() = 1;
  *cfg.vlans_ref()[1].id_ref() = 55;
  *cfg.vlans_ref()[1].name_ref() = "Vlan55";
  cfg.vlans_ref()[1].intfID_ref() = 55;

  cfg.vlanPorts_ref()->resize(20);
  for (int p = 0; p < kPortCount; ++p) {
    *cfg.vlanPorts_ref()[p].logicalPort_ref() = p + 1;
    *cfg.vlanPorts_ref()[p].vlanID_ref() = p < 11 ? 1 : 55;
  }

  cfg.interfaces_ref()->resize(2);
  *cfg.interfaces_ref()[0].intfID_ref() = 1;
  *cfg.interfaces_ref()[0].routerID_ref() = 0;
  *cfg.interfaces_ref()[0].vlanID_ref() = 1;
  cfg.interfaces_ref()[0].name_ref() = "interface1";
  cfg.interfaces_ref()[0].mac_ref() = "00:02:00:00:00:01";
  cfg.interfaces_ref()[0].mtu_ref() = 9000;
  cfg.interfaces_ref()[0].ipAddresses_ref()->resize(3);
  cfg.interfaces_ref()[0].ipAddresses_ref()[0] = "10.0.0.1/24";
  cfg.interfaces_ref()[0].ipAddresses_ref()[1] = "192.168.0.1/24";
  cfg.interfaces_ref()[0].ipAddresses_ref()[2] = "2401:db00:2110:3001::0001/64";

  *cfg.interfaces_ref()[1].intfID_ref() = 55;
  *cfg.interfaces_ref()[1].routerID_ref() = 0;
  *cfg.interfaces_ref()[1].vlanID_ref() = 55;
  cfg.interfaces_ref()[1].name_ref() = "interface55";
  cfg.interfaces_ref()[1].mac_ref() = "00:02:00:00:00:55";
  cfg.interfaces_ref()[1].mtu_ref() = 9000;
  cfg.interfaces_ref()[1].ipAddresses_ref()->resize(3);
  cfg.interfaces_ref()[1].ipAddresses_ref()[0] = "10.0.55.1/24";
  cfg.interfaces_ref()[1].ipAddresses_ref()[1] = "192.168.55.1/24";
  cfg.interfaces_ref()[1].ipAddresses_ref()[2] = "2401:db00:2110:3055::0001/64";

  return cfg;
}

shared_ptr<SwitchState> testState(cfg::SwitchConfig cfg) {
  auto state = make_shared<SwitchState>();

  for (auto cfgVlan : *cfg.vlans_ref()) {
    auto vlan =
        make_shared<Vlan>(VlanID(*cfgVlan.id_ref()), *cfgVlan.name_ref());
    state->addVlan(vlan);
  }

  for (auto cfgVlanPort : *cfg.vlanPorts_ref()) {
    state->registerPort(
        PortID(*cfgVlanPort.logicalPort_ref()),
        folly::to<string>("port", *cfgVlanPort.logicalPort_ref()));
    auto vlan = state->getVlans()->getVlanIf(VlanID(*cfgVlanPort.vlanID_ref()));
    if (vlan) {
      vlan->addPort(PortID(*cfgVlanPort.logicalPort_ref()), false);
    }
  }

  for (auto cfgInterface : *cfg.interfaces_ref()) {
    auto interface = make_shared<Interface>(
        InterfaceID(*cfgInterface.intfID_ref()),
        RouterID(0), /* TODO - vrf is 0 */
        VlanID(*cfgInterface.vlanID_ref()),
        cfgInterface.name_ref().value_or({}),
        folly::MacAddress(cfgInterface.mac_ref().value_or({})),
        cfgInterface.mtu_ref().value_or({}),
        false, /* is virtual */
        false /* is state_sync disabled */
    );

    Interface::Addresses addrs;
    for (auto cfgIpAddress : *cfgInterface.ipAddresses_ref()) {
      /** TODO - fill IPs **/
      std::vector<std::string> splitVec;
      folly::split("/", cfgIpAddress, splitVec);
      addrs.emplace(IPAddress(splitVec[0]), folly::to<uint8_t>(splitVec[1]));
    }
    interface->setAddresses(addrs);

    state->addIntf(interface);

    auto vlan =
        state->getVlans()->getVlanIf(VlanID(*cfgInterface.vlanID_ref()));
    if (vlan) {
      vlan->setInterfaceID(interface->getID());
    }
  }
  RouteUpdater routeUpdater(state->getRouteTables());
  routeUpdater.addInterfaceAndLinkLocalRoutes(state->getInterfaces());

  state->resetRouteTables(routeUpdater.updateDone());
  return state;
}

shared_ptr<SwitchState> testStateA() {
  // Setup a default state object
  auto state = make_shared<SwitchState>();

  // Add VLAN 1, and ports 1-10 which belong to it.
  auto vlan1 = make_shared<Vlan>(VlanID(1), "Vlan1");
  state->addVlan(vlan1);
  for (int idx = 1; idx <= 10; ++idx) {
    state->registerPort(PortID(idx), folly::to<string>("port", idx));
    vlan1->addPort(PortID(idx), false);
    auto port = state->getPorts()->getPortIf(PortID(idx));
    port->addVlan(vlan1->getID(), false);
  }
  // Add VLAN 55, and ports 11-20 which belong to it.
  auto vlan55 = make_shared<Vlan>(VlanID(55), "Vlan55");
  state->addVlan(vlan55);
  for (int idx = 11; idx <= 20; ++idx) {
    state->registerPort(PortID(idx), folly::to<string>("port", idx));
    vlan55->addPort(PortID(idx), false);
    auto port = state->getPorts()->getPortIf(PortID(idx));
    port->addVlan(vlan55->getID(), false);
  }
  // Add Interface 1 to VLAN 1
  auto intf1 = make_shared<Interface>(
      InterfaceID(1),
      RouterID(0),
      VlanID(1),
      "interface1",
      MacAddress("00:02:00:00:00:01"),
      9000,
      false, /* is virtual */
      false /* is state_sync disabled */);
  Interface::Addresses addrs1;
  addrs1.emplace(IPAddress("10.0.0.1"), 24);
  addrs1.emplace(IPAddress("192.168.0.1"), 24);
  addrs1.emplace(IPAddress("2401:db00:2110:3001::0001"), 64);
  intf1->setAddresses(addrs1);
  state->addIntf(intf1);
  vlan1->setInterfaceID(InterfaceID(1));

  // Add Interface 55 to VLAN 55
  auto intf55 = make_shared<Interface>(
      InterfaceID(55),
      RouterID(0),
      VlanID(55),
      "interface55",
      MacAddress("00:02:00:00:00:55"),
      9000,
      false, /* is virtual */
      false /* is state_sync disabled */);
  Interface::Addresses addrs55;
  addrs55.emplace(IPAddress("10.0.55.1"), 24);
  addrs55.emplace(IPAddress("192.168.55.1"), 24);
  addrs55.emplace(IPAddress("2401:db00:2110:3055::0001"), 64);
  intf55->setAddresses(addrs55);
  state->addIntf(intf55);
  vlan55->setInterfaceID(InterfaceID(55));

  RouteUpdater updater(state->getRouteTables());
  updater.addInterfaceAndLinkLocalRoutes(state->getInterfaces());

  RouteNextHopSet nexthops;
  // resolved by intf 1
  nexthops.emplace(
      UnresolvedNextHop(IPAddress("10.0.0.22"), UCMP_DEFAULT_WEIGHT));
  // resolved by intf 1
  nexthops.emplace(
      UnresolvedNextHop(IPAddress("10.0.0.23"), UCMP_DEFAULT_WEIGHT));
  // un-resolvable
  nexthops.emplace(
      UnresolvedNextHop(IPAddress("1.1.2.10"), UCMP_DEFAULT_WEIGHT));

  updater.addRoute(
      RouterID(0),
      IPAddress("10.1.1.0"),
      24,
      ClientID(1001),
      RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));

  auto newRt = updater.updateDone();
  state->resetRouteTables(newRt);

  return state;
}

shared_ptr<SwitchState> testStateAWithPortsUp() {
  return bringAllPortsUp(testStateA());
}

shared_ptr<SwitchState> testStateAWithLookupClasses() {
  auto newState = testStateAWithPortsUp()->clone();
  auto newPortMap = newState->getPorts()->modify(&newState);
  for (auto port : *newPortMap) {
    auto newPort = port->clone();
    newPort->setLookupClassesToDistributeTrafficOn({
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4,
    });
    newPortMap->updatePort(newPort);
  }

  return newState;
}

shared_ptr<SwitchState> testStateB() {
  // Setup a default state object
  auto state = make_shared<SwitchState>();

  // Add VLAN 1, and ports 1-9 which belong to it.
  auto vlan1 = make_shared<Vlan>(VlanID(1), "Vlan1");
  state->addVlan(vlan1);
  for (int idx = 1; idx <= 10; ++idx) {
    state->registerPort(PortID(idx), folly::to<string>("port", idx));
    vlan1->addPort(PortID(idx), false);
  }

  // Add Interface 1 to VLAN 1
  auto intf1 = make_shared<Interface>(
      InterfaceID(1),
      RouterID(0),
      VlanID(1),
      "interface1",
      MacAddress("00:02:00:00:00:01"),
      9000,
      false, /* is virtual */
      false /* is state_sync disabled */);
  Interface::Addresses addrs1;
  addrs1.emplace(IPAddress("10.0.0.1"), 24);
  addrs1.emplace(IPAddress("192.168.0.1"), 24);
  addrs1.emplace(IPAddress("2401:db00:2110:3001::0001"), 64);
  intf1->setAddresses(addrs1);
  state->addIntf(intf1);
  vlan1->setInterfaceID(InterfaceID(1));
  RouteUpdater updater(state->getRouteTables());
  updater.addInterfaceAndLinkLocalRoutes(state->getInterfaces());
  auto newRt = updater.updateDone();
  state->resetRouteTables(newRt);
  return state;
}

shared_ptr<SwitchState> testStateBWithPortsUp() {
  return bringAllPortsUp(testStateB());
}

std::string fbossHexDump(const IOBuf* buf) {
  Cursor cursor(buf);
  size_t length = cursor.totalLength();
  if (length == 0) {
    return "";
  }

  string result;
  result.reserve(length * 3);
  folly::format(&result, "{:02x}", cursor.read<uint8_t>());
  for (size_t n = 1; n < length; ++n) {
    folly::format(&result, " {:02x}", cursor.read<uint8_t>());
  }

  return result;
}

std::string fbossHexDump(const IOBuf& buf) {
  return fbossHexDump(&buf);
}

std::string fbossHexDump(folly::ByteRange buf) {
  IOBuf iobuf(IOBuf::WRAP_BUFFER, buf);
  return fbossHexDump(&iobuf);
}

std::string fbossHexDump(folly::StringPiece buf) {
  IOBuf iobuf(IOBuf::WRAP_BUFFER, ByteRange(buf));
  return fbossHexDump(&iobuf);
}

std::string fbossHexDump(const string& buf) {
  IOBuf iobuf(IOBuf::WRAP_BUFFER, ByteRange(StringPiece(buf)));
  return fbossHexDump(&iobuf);
}

TxPacketMatcher::TxPacketMatcher(StringPiece name, TxMatchFn fn)
    : name_(name.str()), fn_(std::move(fn)) {}

::testing::Matcher<TxPacketPtr> TxPacketMatcher::createMatcher(
    folly::StringPiece name,
    TxMatchFn&& fn) {
  return ::testing::MakeMatcher(new TxPacketMatcher(name, std::move(fn)));
}

#ifndef IS_OSS
bool TxPacketMatcher::MatchAndExplain(
    const TxPacketPtr& pkt,
    ::testing::MatchResultListener* l) const {
#else
bool TxPacketMatcher::MatchAndExplain(
    TxPacketPtr pkt,
    ::testing::MatchResultListener* l) const {
#endif
  try {
    fn_(pkt);
    return true;
  } catch (const std::exception& ex) {
    *l << ex.what();
    return false;
  }
}

void TxPacketMatcher::DescribeTo(std::ostream* os) const {
  *os << name_;
}

void TxPacketMatcher::DescribeNegationTo(std::ostream* os) const {
  *os << "not " << name_;
}

RxPacketMatcher::RxPacketMatcher(
    StringPiece name,
    InterfaceID dstIfID,
    RxMatchFn fn)
    : name_(name.str()), dstIfID_(dstIfID), fn_(std::move(fn)) {}

::testing::Matcher<RxMatchFnArgs> RxPacketMatcher::createMatcher(
    folly::StringPiece name,
    InterfaceID dstIfID,
    RxMatchFn&& fn) {
  return ::testing::MakeMatcher(
      new RxPacketMatcher(name, dstIfID, std::move(fn)));
}

#ifndef IS_OSS
bool RxPacketMatcher::MatchAndExplain(
    const RxMatchFnArgs& args,
    ::testing::MatchResultListener* l) const {
#else
bool RxPacketMatcher::MatchAndExplain(
    RxMatchFnArgs args,
    ::testing::MatchResultListener* l) const {
#endif
  auto dstIfID = std::get<0>(args);
  auto pkt = std::get<1>(args);

  try {
    if (dstIfID != dstIfID_) {
      throw FbossError(
          "Mismatching dstIfID. Expected ",
          dstIfID_,
          " but received ",
          dstIfID);
    }

    fn_(pkt.get());
    return true;
  } catch (const std::exception& ex) {
    *l << ex.what();
    return false;
  }
}

void RxPacketMatcher::DescribeTo(std::ostream* os) const {
  *os << name_;
}

void RxPacketMatcher::DescribeNegationTo(std::ostream* os) const {
  *os << "not " << name_;
}

RouteNextHopSet makeNextHops(std::vector<std::string> ipStrs) {
  RouteNextHopSet nhops;
  for (const std::string& ip : ipStrs) {
    nhops.emplace(UnresolvedNextHop(IPAddress(ip), ECMP_WEIGHT));
  }
  return nhops;
}

RoutePrefixV4 makePrefixV4(std::string str) {
  std::vector<std::string> vec;
  folly::split("/", str, vec);
  EXPECT_EQ(2, vec.size());
  auto prefix =
      RoutePrefixV4{IPAddressV4(vec.at(0)), folly::to<uint8_t>(vec.at(1))};
  return prefix;
}

RoutePrefixV6 makePrefixV6(std::string str) {
  std::vector<std::string> vec;
  folly::split("/", str, vec);
  EXPECT_EQ(2, vec.size());
  auto prefix =
      RoutePrefixV6{IPAddressV6(vec.at(0)), folly::to<uint8_t>(vec.at(1))};
  return prefix;
}

std::shared_ptr<Route<IPAddressV4>> GET_ROUTE_V4(
    const std::shared_ptr<RouteTableMap>& tables,
    RouterID rid,
    RoutePrefixV4 prefix) {
  EXPECT_NE(nullptr, tables);
  auto table = tables->getRouteTableIf(rid);
  EXPECT_NE(nullptr, table);
  auto rib4 = table->getRibV4();
  EXPECT_NE(nullptr, rib4);
  auto rt = rib4->exactMatch(prefix);
  EXPECT_NE(nullptr, rt);
  return rt;
}

std::shared_ptr<Route<IPAddressV4>> GET_ROUTE_V4(
    const std::shared_ptr<RouteTableMap>& tables,
    RouterID rid,
    std::string prefixStr) {
  return GET_ROUTE_V4(tables, rid, makePrefixV4(prefixStr));
}

std::shared_ptr<Route<IPAddressV6>> GET_ROUTE_V6(
    const std::shared_ptr<RouteTableMap>& tables,
    RouterID rid,
    RoutePrefixV6 prefix) {
  EXPECT_NE(nullptr, tables);
  auto table = tables->getRouteTableIf(rid);
  EXPECT_NE(nullptr, table);
  auto rib6 = table->getRibV6();
  EXPECT_NE(nullptr, rib6);
  auto rt = rib6->exactMatch(prefix);
  EXPECT_NE(nullptr, rt);
  return rt;
}

std::shared_ptr<Route<IPAddressV6>> GET_ROUTE_V6(
    const std::shared_ptr<RouteTableMap>& tables,
    RouterID rid,
    std::string prefixStr) {
  return GET_ROUTE_V6(tables, rid, makePrefixV6(prefixStr));
}

void EXPECT_NO_ROUTE(
    const std::shared_ptr<RouteTableMap>& tables,
    RouterID rid,
    std::string prefixStr) {
  // Figure out if it's v4 or v6
  std::vector<std::string> vec;
  folly::split("/", prefixStr, vec);
  EXPECT_EQ(2, vec.size());
  IPAddress ip(vec.at(0));

  if (ip.isV4()) {
    auto prefix =
        RoutePrefixV4{IPAddressV4(vec.at(0)), folly::to<uint8_t>(vec.at(1))};
    EXPECT_EQ(
        nullptr, tables->getRouteTableIf(rid)->getRibV4()->exactMatch(prefix));
  } else {
    auto prefix =
        RoutePrefixV6{IPAddressV6(vec.at(0)), folly::to<uint8_t>(vec.at(1))};
    EXPECT_EQ(
        nullptr, tables->getRouteTableIf(rid)->getRibV6()->exactMatch(prefix));
  }
}

WaitForSwitchState::WaitForSwitchState(
    SwSwitch* sw,
    SwitchStatePredicate predicate,
    const std::string& name)
    : AutoRegisterStateObserver(sw, name),
      predicate_(std::move(predicate)),
      name_(name) {}

WaitForSwitchState::~WaitForSwitchState() {}

void WaitForSwitchState::stateUpdated(const StateDelta& delta) {
  if (predicate_(delta)) {
    {
      std::lock_guard<std::mutex> guard(mtx_);
      done_ = true;
    }
    cv_.notify_all();
  }
}

} // namespace facebook::fboss
