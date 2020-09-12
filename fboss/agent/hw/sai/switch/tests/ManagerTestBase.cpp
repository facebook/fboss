/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/platforms/sai/SaiFakePlatform.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/MacTable.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include <folly/Singleton.h>

#if !defined(IS_OSS) && __has_feature(address_sanitizer)
#include <sanitizer/lsan_interface.h>
#endif

namespace {
facebook::fboss::cfg::AgentConfig getDummyConfig() {
  facebook::fboss::cfg::AgentConfig config;

  config.platform_ref()->platformSettings_ref() = {};
  config.platform_ref()->platformSettings_ref()->insert(std::make_pair(
      facebook::fboss::cfg::PlatformAttributes::CONNECTION_HANDLE,
      "test connection handle"));
  return config;
}
} // namespace

namespace facebook::fboss {

ManagerTestBase::~ManagerTestBase() {}

void ManagerTestBase::SetUp() {
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  setupSaiPlatform();
}

void ManagerTestBase::setupSaiPlatform() {
  auto productInfo = fakeProductInfo();
  saiPlatform = std::make_unique<SaiFakePlatform>(std::move(productInfo));
  auto thriftAgentConfig = getDummyConfig();
  auto agentConfig = std::make_unique<AgentConfig>(
      std::move(thriftAgentConfig), "dummyConfigStr");
  saiPlatform->init(
      std::move(agentConfig),
      (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
       HwSwitch::FeaturesDesired::LINKSCAN_DESIRED));
  saiPlatform->getHwSwitch()->init(nullptr);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  saiPlatform->initPorts();
  saiApiTable = SaiApiTable::getInstance();
  saiManagerTable = saiSwitch->managerTable();
  SwitchSaiId switchId = saiManagerTable->switchManager().getSwitchSaiId();

  auto setupState = std::make_shared<SwitchState>();
  for (int i = 0; i < testInterfaces.size(); ++i) {
    if (i == 0) {
      testInterfaces[i] = TestInterface{i, 4};
    } else {
      testInterfaces[i] = TestInterface{i, 1};
    }
  }

  if (setupStage & SetupStage::PORT) {
    for (const auto& testInterface : testInterfaces) {
      for (const auto& remoteHost : testInterface.remoteHosts) {
        auto swPort = makePort(remoteHost.port);
        setupState->getPorts()->addPort(swPort);
      }
    }
  }
  if (setupStage & SetupStage::VLAN) {
    for (const auto& testInterface : testInterfaces) {
      auto swVlan = makeVlan(testInterface);
      setupState->getVlans()->addVlan(swVlan);
    }
  }
  if (setupStage & SetupStage::INTERFACE) {
    for (const auto& testInterface : testInterfaces) {
      auto swInterface = makeInterface(testInterface);
      setupState->getInterfaces()->addInterface(swInterface);
    }
  }
  if (setupStage & SetupStage::NEIGHBOR) {
    for (const auto& testInterface : testInterfaces) {
      for (const auto& remoteHost : testInterface.remoteHosts) {
        auto swNeighbor = makeArpEntry(testInterface.id, remoteHost);
        auto vlan = setupState->getVlans()->getVlan(VlanID(testInterface.id));
        auto arpTable = vlan->getArpTable();
        PortDescriptor portDesc(PortID(remoteHost.port.id));
        arpTable->addEntry(
            remoteHost.ip.asV4(),
            remoteHost.mac,
            portDesc,
            InterfaceID(testInterface.id));
        auto macTable = vlan->getMacTable();
        macTable->addEntry(std::make_shared<MacEntry>(
            remoteHost.mac,
            portDesc,
            std::nullopt,
            MacEntryType::STATIC_ENTRY));
      }
    }
  }
  applyNewState(setupState);
}

void ManagerTestBase::TearDown() {
  saiPlatform.reset();
  SaiStore::getInstance()->release();
  FakeSai::clear();
  programmedState.reset();
}

void ManagerTestBase::pseudoWarmBootExitAndStoreReload() {
  SaiStore::getInstance()->exitForWarmBoot();
#if !defined(IS_OSS) && __has_feature(address_sanitizer)
  auto* leakedPlatform = saiPlatform.release();
  __lsan_ignore_object(leakedPlatform);
#else
  saiPlatform.release();
#endif
  SaiStore::getInstance()->reload();
}

std::shared_ptr<Port> ManagerTestBase::makePort(
    const TestPort& testPort) const {
  std::string name = folly::sformat("port{}", testPort.id);
  auto swPort = std::make_shared<Port>(PortID(testPort.id), name);
  if (testPort.enabled) {
    swPort->setAdminState(cfg::PortState::ENABLED);
  }
  // Ports are assigned in starting a (vlan/interfaceId * 10 + portIdx),
  // so compute the vlanID based on that.
  VlanID vlan(testPort.id / 10);
  swPort->setIngressVlan(vlan);
  PortFields::VlanMembership vlanMemberShip{
      {vlan, PortFields::VlanInfo{false}}};
  swPort->setVlans(vlanMemberShip);
  swPort->setSpeed(testPort.portSpeed);
  switch (testPort.portSpeed) {
    case cfg::PortSpeed::DEFAULT:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_DEFAULT);
      break;
    case cfg::PortSpeed::GIGE:
      throw FbossError("profile gig ethernet is not available");
      break;
    case cfg::PortSpeed::XG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL);
      break;
    case cfg::PortSpeed::TWENTYG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC);
      break;
    case cfg::PortSpeed::TWENTYFIVEG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL);
      break;
    case cfg::PortSpeed::FORTYG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL);
      break;
    case cfg::PortSpeed::FIFTYG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER);
      break;
    case cfg::PortSpeed::HUNDREDG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL);
      break;
    case cfg::PortSpeed::TWOHUNDREDG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N);
      break;
    case cfg::PortSpeed::FOURHUNDREDG:
      swPort->setProfileId(cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N);
      break;
  }
  return swPort;
}

std::shared_ptr<Vlan> ManagerTestBase::makeVlan(
    const TestInterface& testInterface) const {
  std::string name = folly::sformat("vlan{}", testInterface.id);
  auto swVlan = std::make_shared<Vlan>(VlanID(testInterface.id), name);
  VlanFields::MemberPorts mps;
  for (const auto& remoteHost : testInterface.remoteHosts) {
    PortID portId(remoteHost.port.id);
    VlanFields::PortInfo portInfo(false);
    mps.insert(std::make_pair(portId, portInfo));
  }
  swVlan->setPorts(mps);
  return swVlan;
}

std::shared_ptr<Interface> ManagerTestBase::makeInterface(
    const TestInterface& testInterface) const {
  auto interface = std::make_shared<Interface>(
      InterfaceID(testInterface.id),
      RouterID(0),
      VlanID(testInterface.id),
      folly::sformat("intf{}", testInterface.id),
      testInterface.routerMac,
      1500, // mtu
      false, // isVirtual
      false); // isStateSyncDisabled
  Interface::Addresses addresses;
  addresses.emplace(
      testInterface.subnet.first.asV4(), testInterface.subnet.second);
  interface->setAddresses(addresses);
  return interface;
}

std::shared_ptr<ArpEntry> ManagerTestBase::makePendingArpEntry(
    int id,
    const TestRemoteHost& testRemoteHost) const {
  return std::make_shared<ArpEntry>(
      testRemoteHost.ip.asV4(), InterfaceID(id), NeighborState::PENDING);
}

std::shared_ptr<ArpEntry> ManagerTestBase::makeArpEntry(
    int id,
    const TestRemoteHost& testRemoteHost,
    std::optional<sai_uint32_t> metadata) const {
  auto arpEntry = std::make_shared<ArpEntry>(
      testRemoteHost.ip.asV4(),
      testRemoteHost.mac,
      PortDescriptor(PortID(testRemoteHost.port.id)),
      InterfaceID(id));
  if (metadata) {
    arpEntry->setClassID(static_cast<cfg::AclLookupClass>(metadata.value()));
  }
  return arpEntry;
}

std::shared_ptr<ArpEntry> ManagerTestBase::resolveArp(
    int id,
    const TestRemoteHost& testRemoteHost,
    std::optional<sai_uint32_t> metadata) {
  auto arpEntry = makeArpEntry(id, testRemoteHost, metadata);
  saiManagerTable->neighborManager().addNeighbor(arpEntry);
  saiManagerTable->fdbManager().addFdbEntry(
      arpEntry->getPort().phyPortID(),
      arpEntry->getIntfID(),
      arpEntry->getMac(),
      SAI_FDB_ENTRY_TYPE_STATIC,
      metadata);
  return arpEntry;
}

ResolvedNextHop ManagerTestBase::makeNextHop(
    const TestInterface& testInterface) const {
  const auto& remote = testInterface.remoteHosts.at(0);
  return ResolvedNextHop{remote.ip, InterfaceID(testInterface.id), ECMP_WEIGHT};
}

ResolvedNextHop ManagerTestBase::makeMplsNextHop(
    const TestInterface& testInterface,
    const LabelForwardingAction& action) const {
  const auto& remote = testInterface.remoteHosts.at(0);
  return ResolvedNextHop{
      remote.ip, InterfaceID(testInterface.id), ECMP_WEIGHT, action};
}

std::shared_ptr<Route<folly::IPAddressV4>> ManagerTestBase::makeRoute(
    const TestRoute& route) const {
  RouteFields<folly::IPAddressV4>::Prefix destination;
  destination.network = route.destination.first.asV4();
  destination.mask = route.destination.second;
  RouteNextHopEntry::NextHopSet swNextHops{};
  for (const auto& testInterface : route.nextHopInterfaces) {
    swNextHops.emplace(makeNextHop(testInterface));
  }
  RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(destination);
  r->update(ClientID{42}, entry);
  r->setResolved(entry);
  return r;
}

std::vector<QueueSaiId> ManagerTestBase::getPortQueueSaiIds(
    const SaiPortHandle* portHandle) {
  SaiPortTraits::Attributes::QosQueueList queueListAttribute;
  auto queueSaiIdList = SaiApiTable::getInstance()->portApi().getAttribute(
      portHandle->port->adapterKey(), queueListAttribute);
  std::vector<QueueSaiId> queueSaiIds;
  for (auto queueSaiId : queueSaiIdList) {
    queueSaiIds.push_back(QueueSaiId(queueSaiId));
  }
  return queueSaiIds;
}

std::shared_ptr<PortQueue> ManagerTestBase::makePortQueue(
    uint8_t queueId,
    cfg::StreamType streamType,
    cfg::QueueScheduling schedType,
    uint8_t weight,
    uint64_t minPps,
    uint64_t maxPps) {
  auto portQueue = std::make_shared<PortQueue>(queueId);
  std::string queueName = "queue";
  queueName.append(std::to_string(queueId));
  portQueue->setStreamType(streamType);
  portQueue->setName(queueName);
  cfg::PortQueueRate portQueueRate;
  cfg::Range range;
  *range.minimum_ref() = minPps;
  *range.maximum_ref() = maxPps;
  portQueueRate.set_pktsPerSec(range);
  portQueue->setPortQueueRate(portQueueRate);
  portQueue->setWeight(weight);
  portQueue->setScheduling(schedType);
  return portQueue;
}

QueueConfig ManagerTestBase::makeQueueConfig(
    std::vector<uint8_t> queueIds,
    cfg::StreamType streamType,
    cfg::QueueScheduling schedType,
    uint8_t weight,
    uint64_t minPps,
    uint64_t maxPps) {
  QueueConfig queueConfig;
  for (auto queueId : queueIds) {
    auto portQueue =
        makePortQueue(queueId, streamType, schedType, weight, minPps, maxPps);
    queueConfig.push_back(portQueue);
  }
  return queueConfig;
}

std::shared_ptr<QosPolicy> ManagerTestBase::makeQosPolicy(
    std::string name,
    const TestQosPolicy& qosPolicy) {
  DscpMap dscpMap;
  ExpMap expMap;
  QosPolicyFields::TrafficClassToQueueId tc2q;
  for (const auto& qosMapping : qosPolicy) {
    auto [dscp, tc, q] = qosMapping;
    dscpMap.addFromEntry(TrafficClass{tc}, DSCP{dscp});
    tc2q[TrafficClass(tc)] = q;
  }
  return std::make_shared<QosPolicy>(name, dscpMap, expMap, tc2q);
}

void ManagerTestBase::applyNewState(
    const std::shared_ptr<SwitchState>& newState) {
  auto oldState =
      programmedState ? programmedState : std::make_shared<SwitchState>();
  StateDelta delta(oldState, newState);
  EXPECT_TRUE(saiPlatform->getHwSwitch()->isValidStateUpdate(delta));
  saiPlatform->getHwSwitch()->stateChanged(delta);
  programmedState = newState;
  programmedState->publish();
}

} // namespace facebook::fboss
