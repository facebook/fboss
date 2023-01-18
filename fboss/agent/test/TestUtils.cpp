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

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/MockTunManager.h"

#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/rib/RoutingInformationBase.h"

#include <folly/Memory.h>
#include <folly/json.h>
#include <folly/logging/Init.h>
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
using ::testing::ByMove;
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
    SwitchFlags flags) {
  auto platform = createMockPlatform();
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

std::vector<std::string> getLoopbackIps(int64_t switchIdVal) {
  CHECK_LT(switchIdVal, 10) << " Switch Id >= 10, not supported";

  auto v6 = folly::sformat("20{}::1/64", switchIdVal);
  auto v4 = folly::sformat("20{}.0.0.1/24", switchIdVal);
  return {v6, v4};
}

uint16_t recycleSysPortId(const cfg::DsfNode& node) {
  return *node.systemPortRange()->minimum() + 1;
}

void addRecyclePortRif(const cfg::DsfNode& myNode, cfg::SwitchConfig& cfg) {
  cfg::Interface recyclePortRif;
  recyclePortRif.intfID() = recycleSysPortId(myNode);
  recyclePortRif.type() = cfg::InterfaceType::SYSTEM_PORT;
  for (const auto& address : getLoopbackIps(*myNode.switchId())) {
    recyclePortRif.ipAddresses()->push_back(address);
  }
  cfg.interfaces()->push_back(recyclePortRif);
}

cfg::SwitchConfig testConfigAImpl(bool isMhnic, cfg::SwitchType switchType) {
  cfg::SwitchConfig cfg;
  cfg.switchSettings()->switchType() = switchType;
  static constexpr auto kPortCount = 20;
  cfg.ports()->resize(kPortCount);
  // For VOQ switches reserve port id 1 for recycle port
  const auto kInterfacePortIdBegin =
      (switchType == cfg::SwitchType::VOQ ? 5 : 1);
  for (int p = 0; p < kPortCount; ++p) {
    cfg.ports()[p].logicalID() = p + kInterfacePortIdBegin;
    cfg.ports()[p].name() = folly::to<string>("port", p + 1);
    if (isMhnic) {
      cfg.ports()[p].state() = cfg::PortState::ENABLED;
      cfg.ports()[p].lookupClasses() = {
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4,
      };
      cfg.ports()[p].speed() = cfg::PortSpeed::TWENTYFIVEG;
      cfg.ports()[p].profileID() =
          cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER;
    } else {
      bool isControllingPort = (p % 4 == 0);
      if (isControllingPort) {
        cfg.ports()[p].state() = cfg::PortState::ENABLED;
        cfg.ports()[p].speed() = cfg::PortSpeed::HUNDREDG;
        cfg.ports()[p].profileID() =
            cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER;
      } else {
        cfg.ports()[p].state() = cfg::PortState::DISABLED;
        cfg.ports()[p].speed() = cfg::PortSpeed::TWENTYFIVEG;
        cfg.ports()[p].profileID() =
            cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER;
      }
    }
  }

  if (switchType == cfg::SwitchType::NPU) {
    cfg.vlans()->resize(2);
    cfg.vlans()[0].id() = 1;
    cfg.vlans()[0].name() = "Vlan1";
    cfg.vlans()[0].intfID() = 1;
    cfg.vlans()[1].id() = 55;
    cfg.vlans()[1].name() = "Vlan55";
    cfg.vlans()[1].intfID() = 55;

    cfg.vlanPorts()->resize(20);
    for (int p = 0; p < kPortCount; ++p) {
      cfg.vlanPorts()[p].logicalPort() = p + kInterfacePortIdBegin;
      cfg.vlanPorts()[p].vlanID() = p < 10 + kInterfacePortIdBegin ? 1 : 55;
    }
  } else {
    cfg.vlans()->resize(1);
    cfg.vlans()[0].id() = 0;
    cfg.vlans()[0].name() = "Vlan0";
  }

  cfg.interfaces()->resize(2);
  cfg.interfaces()[0].intfID() = 1;
  cfg.interfaces()[0].routerID() = 0;
  if (switchType == cfg::SwitchType::NPU) {
    cfg.interfaces()[0].vlanID() = 1;
  }
  cfg.interfaces()[0].name() = "fboss1";
  cfg.interfaces()[0].mac() = "00:02:00:00:00:01";
  cfg.interfaces()[0].mtu() = 9000;
  cfg.interfaces()[0].ipAddresses()->resize(4);
  cfg.interfaces()[0].ipAddresses()[0] = "10.0.0.1/24";
  cfg.interfaces()[0].ipAddresses()[1] = "192.168.0.1/24";
  cfg.interfaces()[0].ipAddresses()[2] = "2401:db00:2110:3001::0001/64";
  cfg.interfaces()[0].ipAddresses()[3] = "fe80::/64"; // link local

  cfg.interfaces()[1].intfID() = 55;
  cfg.interfaces()[1].routerID() = 0;
  if (switchType == cfg::SwitchType::NPU) {
    cfg.interfaces()[1].vlanID() = 55;
  }
  cfg.interfaces()[1].name() = "fboss55";
  cfg.interfaces()[1].mac() = "00:02:00:00:00:55";
  cfg.interfaces()[1].mtu() = 9000;
  cfg.interfaces()[1].ipAddresses()->resize(4);
  cfg.interfaces()[1].ipAddresses()[0] = "10.0.55.1/24";
  cfg.interfaces()[1].ipAddresses()[1] = "192.168.55.1/24";
  cfg.interfaces()[1].ipAddresses()[2] = "2401:db00:2110:3055::0001/64";
  cfg.interfaces()[1].ipAddresses()[3] = "169.254.0.0/16"; // link local

  if (switchType != cfg::SwitchType::NPU) {
    cfg.switchSettings()->switchId() = 1;
  }
  if (switchType == cfg::SwitchType::VOQ) {
    cfg::DsfNode myNode = makeDsfNodeCfg(1);
    cfg.dsfNodes()->insert({*myNode.switchId(), myNode});
    cfg::Port recyclePort;
    recyclePort.logicalID() = 1;
    recyclePort.name() = "rcy1/1/1";
    recyclePort.speed() = cfg::PortSpeed::HUNDREDG;
    recyclePort.profileID() =
        cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER;
    recyclePort.portType() = cfg::PortType::RECYCLE_PORT;
    cfg.ports()->push_back(recyclePort);
    addRecyclePortRif(myNode, cfg);
  }
  return cfg;
}

} // namespace

namespace facebook::fboss {

std::shared_ptr<SystemPort> makeSysPort(
    const std::optional<std::string>& qosPolicy,
    int64_t sysPortId,
    int64_t switchId) {
  auto sysPort = std::make_shared<SystemPort>(SystemPortID(sysPortId));
  sysPort->setSwitchId(SwitchID(switchId));
  sysPort->setPortName(folly::sformat("sysPort{}", sysPortId));
  sysPort->setCoreIndex(42);
  sysPort->setCorePortIndex(24);
  sysPort->setSpeedMbps(10000);
  sysPort->setNumVoqs(8);
  sysPort->setEnabled(true);
  sysPort->setQosPolicy(qosPolicy);
  return sysPort;
}
cfg::SwitchConfig updateSwitchID(
    const cfg::SwitchConfig& origCfg,
    int64_t newSwitchId) {
  auto newCfg{origCfg};
  CHECK(origCfg.switchSettings()->switchId());
  newCfg.switchSettings()->switchId() = newSwitchId;
  newCfg.dsfNodes()->insert({newSwitchId, makeDsfNodeCfg(newSwitchId)});
  addRecyclePortRif(newCfg.dsfNodes()->find(newSwitchId)->second, newCfg);
  return newCfg;
}

cfg::DsfNode makeDsfNodeCfg(int64_t switchId, cfg::DsfNodeType type) {
  cfg::DsfNode dsfNodeCfg;
  dsfNodeCfg.switchId() = switchId;
  dsfNodeCfg.name() = folly::sformat("dsfNodeCfg{}", switchId);
  dsfNodeCfg.type() = type;
  const auto kBlockSize{100};
  cfg::Range64 sysPortRange;
  sysPortRange.minimum() = switchId * kBlockSize;
  sysPortRange.maximum() = switchId * kBlockSize + kBlockSize;
  dsfNodeCfg.systemPortRange() = sysPortRange;
  dsfNodeCfg.loopbackIps() = getLoopbackIps(switchId);
  dsfNodeCfg.nodeMac() = "02:00:00:00:0F:0B";
  dsfNodeCfg.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
  return dsfNodeCfg;
}

cfg::SwitchConfig testConfigA(cfg::SwitchType switchType) {
  return testConfigAImpl(false, switchType);
}

cfg::SwitchConfig testConfigAWithLookupClasses() {
  return testConfigAImpl(true, cfg::SwitchType::NPU);
}

shared_ptr<SwitchState> bringAllPortsUp(const shared_ptr<SwitchState>& in) {
  return setAllPortState(in, true);
}
shared_ptr<SwitchState> bringAllPortsDown(const shared_ptr<SwitchState>& in) {
  return setAllPortState(in, false);
}

shared_ptr<SwitchState> removeVlanIPv4Address(
    const shared_ptr<SwitchState>& in,
    VlanID vlanID) {
  auto newState = in->clone();
  for (auto iter : std::as_const(*newState->getInterfaces())) {
    const auto& intf = iter.second;
    if (intf->getVlanID() == vlanID) {
      Interface::Addresses newAddresses;
      for (auto iter : std::as_const(*intf->getAddresses())) {
        auto address = folly::IPAddress(iter.first);
        auto mask = iter.second->cref();
        if (address.isV6()) {
          newAddresses.emplace(address, mask);
        }
      }
      intf->setAddresses(newAddresses);
    }
  }
  return newState;
}

shared_ptr<SwitchState> publishAndApplyConfig(
    const shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    RoutingInformationBase* rib) {
  state->publish();
  return applyThriftConfig(state, config, platform, rib);
}

std::unique_ptr<SwSwitch> setupMockSwitchWithoutHW(
    std::unique_ptr<MockPlatform> platform,
    const std::shared_ptr<SwitchState>& state,
    SwitchFlags flags) {
  // Since we are just applying a initial state and to created
  // switch, set initial config to empty.
  platform->setConfig(createEmptyAgentConfig());
  auto sw = make_unique<SwSwitch>(std::move(platform));
  HwInitResult ret;
  ret.switchState = state ? state : make_shared<SwitchState>();
  ret.bootType = BootType::COLD_BOOT;
  ret.rib = std::make_unique<RoutingInformationBase>();
  EXPECT_HW_CALL(sw, initImpl(_, false, _, _))
      .WillOnce(Return(ByMove(std::move(ret))));
  initSwSwitchWithFlags(sw.get(), flags);
  waitForStateUpdates(sw.get());
  return sw;
}

unique_ptr<MockPlatform> createMockPlatform(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId) {
  auto mock = make_unique<testing::NiceMock<MockPlatform>>();
  cfg::AgentConfig thrift;
  thrift.sw()->switchSettings()->switchType() = switchType;
  if (switchId.has_value()) {
    thrift.sw()->switchSettings()->switchId() = *switchId;
  }
  auto agentCfg = std::make_unique<AgentConfig>(thrift, "");
  mock->init(std::move(agentCfg), 0);
  return std::move(mock);
}

unique_ptr<HwTestHandle> createTestHandle(
    const shared_ptr<SwitchState>& state,
    SwitchFlags flags) {
  auto sw = createMockSw(state, flags);
  auto platform = static_cast<MockPlatform*>(sw->getPlatform());
  auto handle = platform->createTestHandle(std::move(sw));
  handle->prepareForTesting();
  return handle;
}

unique_ptr<HwTestHandle> createTestHandle(
    cfg::SwitchConfig* config,
    SwitchFlags flags) {
  shared_ptr<SwitchState> initialState{nullptr};
  // Create the initial state, which only has the same number of ports with the
  // init config
  initialState = make_shared<SwitchState>();
  if (config) {
    for (const auto& port : *config->ports()) {
      auto id = *port.logicalID();
      initialState->registerPort(PortID(id), folly::to<string>("port", id));
    }
  }

  auto handle = createTestHandle(initialState, flags);
  auto sw = handle->getSw();

  if (config) {
    // Apply the thrift config
    sw->applyConfig("test_setup", *config);
  }
  return handle;
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

void waitForNeighborCacheThread(SwSwitch* sw) {
  auto* evb = sw->getNeighborCacheEvb();
  evb->runInEventBaseThreadAndWait([]() { return; });
}

void waitForRibUpdates(SwSwitch* sw) {
  sw->getRouteUpdater().program();
}

shared_ptr<SwitchState> testStateA() {
  // Setup a default state object
  auto state = make_shared<SwitchState>();

  // Add VLAN 1, and ports 1-10 which belong to it.
  auto vlan1 = make_shared<Vlan>(VlanID(1), std::string("Vlan1"));
  state->addVlan(vlan1);
  for (int idx = 1; idx <= 10; ++idx) {
    state->registerPort(PortID(idx), folly::to<string>("port", idx));
    vlan1->addPort(PortID(idx), false);
    auto port = state->getPorts()->getPortIf(PortID(idx));
    port->addVlan(vlan1->getID(), false);
  }
  // Add VLAN 55, and ports 11-20 which belong to it.
  auto vlan55 = make_shared<Vlan>(VlanID(55), std::string("Vlan55"));
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
      std::optional<VlanID>(1),
      folly::StringPiece("fboss1"),
      MacAddress("00:02:00:00:00:01"),
      9000,
      false, /* is virtual */
      false /* is state_sync disabled */);
  Interface::Addresses addrs1;
  addrs1.emplace(IPAddress("10.0.0.1"), 24);
  addrs1.emplace(IPAddress("192.168.0.1"), 24);
  addrs1.emplace(IPAddress("169.254.0.0"), 16); // link local

  addrs1.emplace(IPAddress("2401:db00:2110:3001::0001"), 64);
  addrs1.emplace(IPAddress("fe80::"), 64); // link local

  intf1->setAddresses(addrs1);
  state->addIntf(intf1);
  vlan1->setInterfaceID(InterfaceID(1));

  // Add Interface 55 to VLAN 55
  auto intf55 = make_shared<Interface>(
      InterfaceID(55),
      RouterID(0),
      std::optional<VlanID>(55),
      folly::StringPiece("fboss55"),
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

shared_ptr<SwitchState> testStateAWithoutIpv4VlanIntf(VlanID vlanId) {
  return removeVlanIPv4Address(testStateA(), vlanId);
}

shared_ptr<SwitchState> testStateB() {
  // Setup a default state object
  auto state = make_shared<SwitchState>();

  // Add VLAN 1, and ports 1-9 which belong to it.
  auto vlan1 = make_shared<Vlan>(VlanID(1), std::string("Vlan1"));
  state->addVlan(vlan1);
  for (int idx = 1; idx <= 10; ++idx) {
    state->registerPort(PortID(idx), folly::to<string>("port", idx));
    vlan1->addPort(PortID(idx), false);
  }

  // Add Interface 1 to VLAN 1
  auto intf1 = make_shared<Interface>(
      InterfaceID(1),
      RouterID(0),
      std::optional<VlanID>(1),
      folly::StringPiece("fboss1"),
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

bool TxPacketMatcher::MatchAndExplain(
    TxPacketPtr pkt,
    ::testing::MatchResultListener* l) const {
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

bool RxPacketMatcher::MatchAndExplain(
    RxMatchFnArgs args,
    ::testing::MatchResultListener* l) const {
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

RouteNextHopSet makeNextHops(
    std::vector<std::string> ipStrs,
    std::optional<LabelForwardingAction> mplsAction) {
  RouteNextHopSet nhops;
  for (const std::string& ip : ipStrs) {
    nhops.emplace(UnresolvedNextHop(IPAddress(ip), ECMP_WEIGHT, mplsAction));
  }
  return nhops;
}
RouteNextHopSet makeResolvedNextHops(
    std::vector<std::pair<InterfaceID, std::string>> intfAndIPs,
    uint32_t weight) {
  RouteNextHopSet nhops;
  for (auto intfAndIP : intfAndIPs) {
    nhops.emplace(
        ResolvedNextHop(IPAddress(intfAndIP.second), intfAndIP.first, weight));
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

WaitForSwitchState::WaitForSwitchState(
    SwSwitch* sw,
    SwitchStatePredicate predicate,
    const std::string& name)
    : predicate_(std::move(predicate)), name_(name), sw_(sw) {
  sw_->registerStateObserver(this, name_);
}

WaitForSwitchState::~WaitForSwitchState() {
  sw_->unregisterStateObserver(this);
}

void WaitForSwitchState::stateUpdated(const StateDelta& delta) {
  if (predicate_(delta)) {
    {
      std::lock_guard<std::mutex> guard(mtx_);
      done_ = true;
    }
    cv_.notify_all();
  }
}

void programRoutes(
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks,
    SwSwitch* sw) {
  auto updater = sw->getRouteUpdater();
  for (const auto& routeChunk : routeChunks) {
    for (const auto& route : routeChunk) {
      RouteNextHopSet nhops;
      for (const auto& nhop : route.nhops) {
        nhops.emplace(nhop);
      }
      updater.addRoute(
          RouterID(0),
          route.prefix.first,
          route.prefix.second,
          ClientID::BGPD,
          RouteNextHopEntry(nhops, AdminDistance::EBGP));
    }
  }
  updater.program();
}

void updateBlockedNeighbor(
    SwSwitch* sw,
    const std::vector<std::pair<VlanID, folly::IPAddress>>& blockNeighbors) {
  sw->updateStateBlocking(
      "Update blocked neighbors ",
      [=](const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};

        auto newSwitchSettings = state->getSwitchSettings()->modify(&newState);
        newSwitchSettings->setBlockNeighbors(blockNeighbors);
        return newState;
      });

  waitForStateUpdates(sw);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(sw);
  waitForStateUpdates(sw);
}

void updateMacAddrsToBlock(
    SwSwitch* sw,
    const std::vector<std::pair<VlanID, folly::MacAddress>>& macAddrsToBlock) {
  sw->updateStateBlocking(
      "Update MAC addrs to block traffic to",
      [=](const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};

        auto newSwitchSettings = state->getSwitchSettings()->modify(&newState);
        newSwitchSettings->setMacAddrsToBlock(macAddrsToBlock);
        return newState;
      });

  waitForStateUpdates(sw);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(sw);
  waitForStateUpdates(sw);
}

std::vector<std::shared_ptr<Port>> getPortsInLoopbackMode(
    const std::shared_ptr<SwitchState>& state) {
  std::vector<std::shared_ptr<Port>> lbPorts;
  for (auto port : *state->getPorts()) {
    if (port->getLoopbackMode() != cfg::PortLoopbackMode::NONE) {
      lbPorts.push_back(port);
    }
  }
  return lbPorts;
}

void preparedMockPortConfig(
    cfg::Port& portCfg,
    int32_t id,
    std::optional<std::string> name,
    cfg::PortState state) {
  portCfg.logicalID() = id;
  portCfg.state() = state;
  portCfg.name() = name ? *name : fmt::format("port{}", id);
  // Always use 10G because all the platform ports from MockPlatform can
  // support such speed and profile
  portCfg.speed() = cfg::PortSpeed::XG;
  portCfg.profileID() = cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER;
}

} // namespace facebook::fboss
