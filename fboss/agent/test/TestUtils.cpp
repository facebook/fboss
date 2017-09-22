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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockableHwSwitch.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockablePlatform.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/test/MockTunManager.h"

#include <folly/Memory.h>
#include <folly/json.h>
#include <folly/Optional.h>

using folly::MacAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::ByteRange;
using folly::IOBuf;
using folly::io::Cursor;
using std::make_unique;
using folly::StringPiece;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

DEFINE_bool(switch_hw, false, "Run tests for actual hw");

namespace facebook { namespace fboss {

namespace {

void initSwSwitchWithFlags(SwSwitch* sw, SwitchFlags flags) {
  if (flags & ENABLE_TUN) {
    // TODO(aeckert): I don't think this should be a first class
    // argument to SwSwitch::init() as unit tests are the only place
    // that pass in a TunManager to init(). Let's come up with a way
    // to mock the TunManager initialization instead of passing it in
    // like this.
    //
    // TODO(aeckert): Have MockTunManager hit the real TunManager
    // implementation if testing on actual hw
    auto mockTunMgr = new MockTunManager(sw, sw->getBackgroundEVB());
    std::unique_ptr<TunManager> tunMgr(mockTunMgr);
    sw->init(std::move(tunMgr), flags);
  } else {
    sw->init(nullptr, flags);
  }
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

std::unique_ptr<SwSwitch> setupMockSwitchWithHW(
    std::unique_ptr<MockPlatform> platform,
    const std::shared_ptr<SwitchState>& state,
    SwitchFlags flags) {
  auto sw = make_unique<SwSwitch>(std::move(platform));
  EXPECT_PLATFORM_CALL(sw, onHwInitialized(_)).Times(1);
  initSwSwitchWithFlags(sw.get(), flags);
  if (state) {
    sw->updateState(
      "Apply initial test state",
      [state](const shared_ptr<SwitchState>& prev) -> shared_ptr<SwitchState> {
        state->inheritGeneration(*prev);
        shared_ptr<SwitchState> newState{state};

        // A bit hacky, but fboss currently expects the sw port
        // mapping to match what is in hardware. Instead of removing
        // any ports not specified in the initial state altogether,
        // just disable non-specified ports that exist in hw.
        auto newPorts = newState->getPorts()->modify(&newState);
        for (auto port : *prev->getPorts()) {
          if (!newPorts->getPortIf(port->getID())) {
            // port not in desired state, add it as a disabled port
            auto newPort = port->clone();
            newPort->setAdminState(cfg::PortState::DISABLED);
            newPorts->addPort(newPort);
          }
        }

        return newState;
      });
  }
  sw->initialConfigApplied(std::chrono::steady_clock::now());
  waitForStateUpdates(sw.get());
  return sw;
}

}

shared_ptr<SwitchState> publishAndApplyConfig(
    shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    const cfg::SwitchConfig* prevCfg) {
  state->publish();
  return applyThriftConfig(state, config, platform, prevCfg);
}

shared_ptr<SwitchState> publishAndApplyConfigFile(
    shared_ptr<SwitchState>& state,
    StringPiece path,
    const Platform* platform,
    std::string prevConfigStr) {
  state->publish();
  // Parse the prev JSON config.
  cfg::SwitchConfig prevConfig;
  if (prevConfigStr.size()) {
    apache::thrift::SimpleJSONSerializer::deserialize<cfg::SwitchConfig>(
        prevConfigStr.c_str(), prevConfig);
  }
  return applyThriftConfigFile(state, path, platform, &prevConfig).first;
}

unique_ptr<MockPlatform> createMockPlatform() {
  if (!FLAGS_switch_hw) {
    return make_unique<testing::NiceMock<MockPlatform>>();
  }

  std::shared_ptr<Platform> platform(initWedgePlatform().release());
  return make_unique<testing::NiceMock<MockablePlatform>>(platform);
}

unique_ptr<SwSwitch> createMockSw(
    const shared_ptr<SwitchState>& state,
    const folly::Optional<MacAddress>& mac,
    SwitchFlags flags) {
  auto platform = createMockPlatform();
  if (mac) {
    EXPECT_CALL(*platform.get(), getLocalMac()).WillRepeatedly(
      Return(mac.value()));
  }

  if (FLAGS_switch_hw) {
    return setupMockSwitchWithHW(std::move(platform), state, flags);
  }
  return setupMockSwitchWithoutHW(std::move(platform), state, flags);
}

unique_ptr<HwTestHandle> createTestHandle(
    const shared_ptr<SwitchState>& state,
    const folly::Optional<MacAddress>& mac,
    SwitchFlags flags) {
  auto sw = createMockSw(state, mac, flags);
  auto platform = sw->getPlatform();
  auto handle = platform->createTestHandle(std::move(sw));
  handle->prepareForTesting();
  return handle;
}

unique_ptr<HwTestHandle> createTestHandle(cfg::SwitchConfig* config,
                                          MacAddress mac,
                                          SwitchFlags flags) {
  shared_ptr<SwitchState> initialState{nullptr};
  if (!FLAGS_switch_hw) {
    // Create the initial state, which only has ports
    initialState = make_shared<SwitchState>();
    uint32_t maxPort{0};
    for (const auto& port : config->ports) {
      maxPort = std::max(static_cast<int32_t>(maxPort), port.logicalID);
    }
    for (uint32_t idx = 1; idx <= maxPort; ++idx) {
      initialState->registerPort(PortID(idx), folly::to<string>("port", idx));
    }
  }

  auto handle = createTestHandle(initialState, mac, flags);
  auto sw = handle->getSw();

  // Apply the thrift config
  auto updateFn = [&](const shared_ptr<SwitchState>& state) {
    return applyThriftConfig(state, config, sw->getPlatform());
  };
  sw->updateStateBlocking("test_setup", updateFn);
  return handle;
}

unique_ptr<HwTestHandle> createTestHandle(cfg::SwitchConfig* config,
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
      -> std::shared_ptr<SwitchState>{
    // take a snapshot of the current state, but don't modify state
    snapshot = state;
    return nullptr;
  };
  sw->updateStateBlocking("waitForStateUpdates", snapshotUpdate);
  return snapshot;
}

void waitForBackgroundThread(SwSwitch* sw) {
  auto* evb = sw->getBackgroundEVB();
  evb->runInEventBaseThreadAndWait([]() { return; });
}

cfg::SwitchConfig testConfigA() {
  cfg::SwitchConfig cfg;
  static constexpr auto kPortCount = 20;

  cfg.ports.resize(kPortCount);
  for (int p = 0; p < kPortCount; ++p) {
    cfg.ports[p].logicalID = p + 1;
    cfg.ports[p].name = folly::to<string>("port", p + 1);
  }

  cfg.vlans.resize(2);
  cfg.vlans[0].id = 1;
  cfg.vlans[0].name = "Vlan1";
  cfg.vlans[0].intfID = 1;
  cfg.vlans[1].id = 55;
  cfg.vlans[1].name = "Vlan55";
  cfg.vlans[1].intfID = 55;

  cfg.vlanPorts.resize(20);
  for (int p = 0; p < kPortCount; ++p) {
    cfg.vlanPorts[p].logicalPort = p + 1;
    cfg.vlanPorts[p].vlanID = p < 11 ? 1 : 55;
  }

  cfg.interfaces.resize(2);
  cfg.interfaces[0].intfID = 1;
  cfg.interfaces[0].routerID = 0;
  cfg.interfaces[0].vlanID = 1;
  cfg.interfaces[0].name = "interface1";
  cfg.interfaces[0].mac = "00:02:00:00:00:01";
  cfg.interfaces[0].mtu = 9000;
  cfg.interfaces[0].ipAddresses.resize(3);
  cfg.interfaces[0].ipAddresses[0] = "10.0.0.1/24";
  cfg.interfaces[0].ipAddresses[1] = "192.168.0.1/24";
  cfg.interfaces[0].ipAddresses[2] = "2401:db00:2110:3001::0001/64";

  cfg.interfaces[1].intfID = 55;
  cfg.interfaces[1].routerID = 0;
  cfg.interfaces[1].vlanID = 55;
  cfg.interfaces[1].name = "interface55";
  cfg.interfaces[1].mac = "00:02:00:00:00:55";
  cfg.interfaces[1].mtu = 9000;
  cfg.interfaces[1].ipAddresses.resize(3);
  cfg.interfaces[1].ipAddresses[0] = "10.0.55.1/24";
  cfg.interfaces[1].ipAddresses[1] = "192.168.55.1/24";
  cfg.interfaces[1].ipAddresses[2] = "2401:db00:2110:3055::0001/64";

  return cfg;
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
  }
  // Add VLAN 55, and ports 11-20 which belong to it.
  auto vlan55 = make_shared<Vlan>(VlanID(55), "Vlan55");
  state->addVlan(vlan55);
  for (int idx = 11; idx <= 20; ++idx) {
    state->registerPort(PortID(idx), folly::to<string>("port", idx));
    vlan55->addPort(PortID(idx), false);
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
      false  /* is state_sync disabled */);
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
      false  /* is state_sync disabled */);
  Interface::Addresses addrs55;
  addrs55.emplace(IPAddress("10.0.55.1"), 24);
  addrs55.emplace(IPAddress("192.168.55.1"), 24);
  addrs55.emplace(IPAddress("2401:db00:2110:3055::0001"), 64);
  intf55->setAddresses(addrs55);
  state->addIntf(intf55);
  vlan55->setInterfaceID(InterfaceID(55));


  RouteUpdater updater(state->getRouteTables());
  updater.addInterfaceAndLinkLocalRoutes(state->getInterfaces());

  RouteNextHops nexthops;
  // resolved by intf 1
  nexthops.emplace(RouteNextHop::createNextHop(IPAddress("10.0.0.22")));
  // resolved by intf 1
  nexthops.emplace(RouteNextHop::createNextHop(IPAddress("10.0.0.23")));
  // un-resolvable
  nexthops.emplace(RouteNextHop::createNextHop(IPAddress("1.1.2.10")));

  updater.addRoute(RouterID(0), IPAddress("10.1.1.0"), 24,
                   ClientID(1001), RouteNextHopEntry(nexthops));

  auto newRt = updater.updateDone();
  state->resetRouteTables(newRt);

  return state;
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
      false  /* is state_sync disabled */);
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
    const TxPacketPtr& pkt, ::testing::MatchResultListener* l) const {
#else
bool TxPacketMatcher::MatchAndExplain(
    TxPacketPtr pkt, ::testing::MatchResultListener* l) const {
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
    StringPiece name, InterfaceID dstIfID, RxMatchFn fn)
    : name_(name.str()), dstIfID_(dstIfID), fn_(std::move(fn)) {}

::testing::Matcher<RxMatchFnArgs>
RxPacketMatcher::createMatcher(
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
          "Mismatching dstIfID. Expected ", dstIfID_,
          " but received ", dstIfID);
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

RouteNextHops makeNextHops(std::vector<std::string> ipStrs) {
  RouteNextHops nhops;
  for (const std::string & ip : ipStrs) {
    nhops.emplace(RouteNextHop::createNextHop(IPAddress(ip)));
  }
  return nhops;
}

RoutePrefixV4 makePrefixV4(std::string str) {
  std::vector<std::string> vec;
  folly::split("/", str, vec);
  EXPECT_EQ(2, vec.size());
  auto prefix = RoutePrefixV4{IPAddressV4(vec.at(0)),
                              folly::to<uint8_t>(vec.at(1))};
  return prefix;
}

RoutePrefixV6 makePrefixV6(std::string str) {
  std::vector<std::string> vec;
  folly::split("/", str, vec);
  EXPECT_EQ(2, vec.size());
  auto prefix = RoutePrefixV6{IPAddressV6(vec.at(0)),
                              folly::to<uint8_t>(vec.at(1))};
  return prefix;
}

std::shared_ptr<Route<IPAddressV4>>
GET_ROUTE_V4(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, RoutePrefixV4 prefix) {
  EXPECT_NE(nullptr, tables);
  auto table = tables->getRouteTableIf(rid);
  EXPECT_NE(nullptr, table);
  auto rib4 = table->getRibV4();
  EXPECT_NE(nullptr, rib4);
  auto rt = rib4->exactMatch(prefix);
  EXPECT_NE(nullptr, rt);
  return rt;
}

std::shared_ptr<Route<IPAddressV4>>
GET_ROUTE_V4(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, std::string prefixStr) {
  return GET_ROUTE_V4(tables, rid, makePrefixV4(prefixStr));
}

std::shared_ptr<Route<IPAddressV6>>
GET_ROUTE_V6(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, RoutePrefixV6 prefix) {
  EXPECT_NE(nullptr, tables);
  auto table = tables->getRouteTableIf(rid);
  EXPECT_NE(nullptr, table);
  auto rib6 = table->getRibV6();
  EXPECT_NE(nullptr, rib6);
  auto rt = rib6->exactMatch(prefix);
  EXPECT_NE(nullptr, rt);
  return rt;
}

std::shared_ptr<Route<IPAddressV6>>
GET_ROUTE_V6(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, std::string prefixStr) {
  return GET_ROUTE_V6(tables, rid, makePrefixV6(prefixStr));
}


void EXPECT_NO_ROUTE(const std::shared_ptr<RouteTableMap>& tables,
                     RouterID rid, std::string prefixStr) {
  // Figure out if it's v4 or v6
  std::vector<std::string> vec;
  folly::split("/", prefixStr, vec);
  EXPECT_EQ(2, vec.size());
  IPAddress ip(vec.at(0));

  if (ip.isV4()) {
    auto prefix = RoutePrefixV4{IPAddressV4(vec.at(0)),
      folly::to<uint8_t>(vec.at(1))};
    EXPECT_EQ(nullptr,
              tables->getRouteTableIf(rid)->getRibV4()->exactMatch(prefix));
  } else {
    auto prefix = RoutePrefixV6{IPAddressV6(vec.at(0)),
      folly::to<uint8_t>(vec.at(1))};
    EXPECT_EQ(nullptr,
              tables->getRouteTableIf(rid)->getRibV6()->exactMatch(prefix));
  }
}

}} // namespace facebook::fboss
