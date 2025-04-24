#include "fboss/agent/test/utils/SystemScaleTestUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/utils/AclScaleTestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/MacLearningFloodHelper.h"
#include "fboss/agent/test/utils/PortFlapHelper.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "folly/Benchmark.h"

DECLARE_bool(intf_nbr_tables);
DECLARE_bool(json);
DECLARE_int32(max_l2_entries);
DECLARE_int32(max_ndp_entries);
DECLARE_int32(max_arp_entries);

DEFINE_string(
    write_agent_config_marker_for_fsdb,
    "",
    "Write marker file for FSDB");

namespace {
constexpr uint64_t kBaseMac = 0xFEEEC2000010;
constexpr uint64_t kNeighborBaseMac = 0xF0EEC2000010;
// the number of rounds to add/churn fboss routes and neighbors is limited by
// the time to run the test. The number of rounds is chosen to finish in test in
// 15mins
constexpr int kNumChurn = 3;
constexpr int kMaxScaleMacTxPortIdx = 1;
constexpr int kMaxScaleMacRxPortIdx = 0;
constexpr int kMacChurnTxPortIdx = 2;
constexpr int kMacChurnRxPortIdx = 3;
constexpr int kRxMeasurePortIdx = 4;
} // namespace

namespace facebook::fboss::utility {

template <typename AddrT>
void removeNeighbor(AgentEnsemble* ensemble) {
  ensemble->applyNewState(
      [&](const std::shared_ptr<SwitchState>& in) {
        auto newState = in->clone();
        if (FLAGS_intf_nbr_tables) {
          auto intfID =
              ensemble->getProgrammedState()
                  ->getPorts()
                  ->getNodeIf(ensemble->masterLogicalInterfacePortIds()[0])
                  ->getInterfaceID();
          Interface* interface =
              newState->getInterfaces()->getNode(intfID).get();
          interface = interface->modify(&newState);
          if (std::is_same<AddrT, folly::IPAddressV4>::value) {
            XLOG(DBG2) << "# arp entries" << interface->getArpTable()->size();
            interface->setArpTable(std::make_shared<ArpTable>());
          } else {
            XLOG(DBG2) << "# ndp entries" << interface->getNdpTable()->size();
            interface->setNdpTable(std::make_shared<NdpTable>());
          }
        } else {
          auto vlanID =
              ensemble->getProgrammedState()
                  ->getPorts()
                  ->getNodeIf(ensemble->masterLogicalInterfacePortIds()[0])
                  ->getIngressVlan();
          Vlan* vlan = newState->getVlans()->getNode(vlanID).get();
          vlan = vlan->modify(&newState);
          if (std::is_same<AddrT, folly::IPAddressV4>::value) {
            XLOG(DBG2) << "# arp entries" << vlan->getArpTable()->size();
            vlan->setArpTable(std::make_shared<ArpTable>());
          } else {
            XLOG(DBG2) << "# ndp entries" << vlan->getNdpTable()->size();
            vlan->setNdpTable(std::make_shared<NdpTable>());
          }
        }
        return newState;
      },
      "remove neighbor",
      false);
}

void removeAllNeighbors(AgentEnsemble* ensemble) {
  removeNeighbor<folly::IPAddressV4>(ensemble);
  removeNeighbor<folly::IPAddressV6>(ensemble);
}

template <typename AddrT>
std::vector<std::pair<AddrT, folly::MacAddress>> neighborAddrs(
    int numNeighbors) {
  std::vector<std::pair<AddrT, folly::MacAddress>> macIPPairs;
  for (int i = 0; i < numNeighbors; ++i) {
    std::stringstream ipStream;

    if (std::is_same<AddrT, folly::IPAddressV6>::value) {
      ipStream << "2001:0db8:85a3:0000:0000:8a2e:" << std::hex
               << (i >> 16 & 0xffff) << ":" << std::hex << (i & 0xffff);
    } else {
      ipStream << "100.100." << (i >> 8 & 0xff) << "." << (i & 0xff);
    }
    AddrT ip(ipStream.str());
    uint64_t macBytes = kNeighborBaseMac + 1;
    folly::MacAddress mac = folly::MacAddress::fromHBO(macBytes);
    macIPPairs.push_back(std::make_pair(ip, mac));
  }
  return macIPPairs;
}

template <typename AddrT>
std::shared_ptr<facebook::fboss::SwitchState> updateNeighborEntry(
    AgentEnsemble* ensemble,
    const std::shared_ptr<SwitchState>& in,
    const PortDescriptor& port,
    const AddrT& addr,
    const folly::MacAddress& mac,
    const bool isIntfNbrTable,
    std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
  using NeighborTableT = typename std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;
  auto state = in->clone();
  NeighborTableT* neighborTable;
  auto intfID = ensemble->getProgrammedState()
                    ->getPorts()
                    ->getNodeIf(ensemble->masterLogicalInterfacePortIds()[0])
                    ->getInterfaceID();

  if (isIntfNbrTable) {
    neighborTable = state->getInterfaces()
                        ->getNode(intfID)
                        ->template getNeighborEntryTable<AddrT>()
                        ->modify(intfID, &state);
  } else {
    auto vlanID = ensemble->getProgrammedState()
                      ->getPorts()
                      ->getNodeIf(ensemble->masterLogicalInterfacePortIds()[0])
                      ->getIngressVlan();
    neighborTable = state->getVlans()
                        ->getNode(vlanID)
                        ->template getNeighborEntryTable<AddrT>()
                        ->modify(vlanID, &state);
  }

  if (neighborTable->getEntryIf(addr)) {
    neighborTable->updateEntry(
        addr, mac, port, intfID, NeighborState::REACHABLE, lookupClass);
  } else {
    neighborTable->addEntry(addr, mac, port, intfID);
    // Update entry to add classid if any
    neighborTable->updateEntry(
        addr, mac, port, intfID, NeighborState::REACHABLE, lookupClass);
  }
  return state;
}

template <typename AddrT>
void programNeighbor(
    AgentEnsemble* ensemble,
    const PortDescriptor& port,
    const AddrT& addr,
    const folly::MacAddress neighborMac,
    std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
  ensemble->applyNewState(
      [&](const std::shared_ptr<SwitchState>& in) {
        return updateNeighborEntry(
            ensemble,
            in,
            port,
            addr,
            neighborMac,
            FLAGS_intf_nbr_tables,
            lookupClass);
      },
      "program neighbor",
      false);
}

void programNeighbors(
    AgentEnsemble* ensemble,
    const PortDescriptor& port,
    std::optional<cfg::AclLookupClass> lookupClass) {
  int numNDPNeighbors, numARPNeighbors;

  numNDPNeighbors = FLAGS_max_ndp_entries;
  numARPNeighbors = FLAGS_max_arp_entries;
  XLOG(DBG2) << "Max NDP neighbors: " << numNDPNeighbors << " Max ARP neighbors"
             << numARPNeighbors;

  for (auto pair : neighborAddrs<folly::IPAddressV4>(numARPNeighbors)) {
    programNeighbor(ensemble, port, pair.first, pair.second, lookupClass);
  }
  for (auto pair : neighborAddrs<folly::IPAddressV6>(numNDPNeighbors)) {
    programNeighbor(ensemble, port, pair.first, pair.second, lookupClass);
  }
}

void configureMaxNeighborEntries(AgentEnsemble* ensemble) {
  programNeighbors(
      ensemble,
      PortDescriptor(ensemble->masterLogicalInterfacePortIds()[0]),
      std::nullopt);
}

std::vector<L2Entry> generateL2Entries(
    AgentEnsemble* ensemble,
    const std::vector<folly::MacAddress>& macs,
    PortID portID) {
  std::vector<L2Entry> l2Entries;
  const auto portDescr = PortDescriptor(portID);
  auto vlan = ensemble->getProgrammedState()
                  ->getPorts()
                  ->getNodeIf(portID)
                  ->getIngressVlan();
  l2Entries.reserve(macs.size());
  for (auto& mac : macs) {
    l2Entries.emplace_back(
        mac, vlan, portDescr, L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
  }
  return l2Entries;
}

void configureMaxMacEntriesViaL2LearningUpdate(AgentEnsemble* ensemble) {
  std::vector<folly::MacAddress> macs;
  // mac learned from port
  auto rxPort =
      PortID(ensemble->masterLogicalInterfacePortIds()[kMaxScaleMacRxPortIdx]);
  auto vlan = ensemble->getProgrammedState()
                  ->getPorts()
                  ->getNodeIf(rxPort)
                  ->getIngressVlan();
  for (int i = 0; i < FLAGS_max_l2_entries; ++i) {
    folly::MacAddress mac = folly::MacAddress::fromHBO(kBaseMac + i);
    macs.push_back(mac);
  }
  auto l2Entries = generateL2Entries(ensemble, macs, rxPort);
  for (int i = 0; i < l2Entries.size(); i++) {
    XLOG(DBG2) << "Adding i " << i << " MAC " << l2Entries[i].str();
    ensemble->getSw()->l2LearningUpdateReceived(
        l2Entries[i], L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);
  }
  int numMacs;
  std::condition_variable cv;
  std::chrono::milliseconds waitForMacInstalled =
      std::chrono::milliseconds(1000);
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);

  do {
    numMacs = ensemble->getProgrammedState()
                  ->getVlans()
                  ->getNodeIf(vlan)
                  ->getMacTable()
                  ->size();
    XLOG(DBG2) << "mac table size: " << numMacs;
    cv.wait_for(lock, waitForMacInstalled, [] { return false; });
  } while (numMacs < FLAGS_max_l2_entries);
}

void configureMaxMacEntriesViaPacketIn(AgentEnsemble* ensemble) {
  CHECK_GE(ensemble->masterLogicalInterfacePortIds().size(), 2);
  PortID txPort =
      PortID(ensemble->masterLogicalInterfacePortIds()[kMaxScaleMacTxPortIdx]);
  PortID rxPort =
      PortID(ensemble->masterLogicalInterfacePortIds()[kMaxScaleMacRxPortIdx]);
  auto vlan = ensemble->getProgrammedState()
                  ->getPorts()
                  ->getNodeIf(rxPort)
                  ->getIngressVlan();

  auto rxInterfaceMac = getInterfaceMac(
      ensemble->getProgrammedState(),
      ensemble->getProgrammedState()
          ->getPorts()
          ->getNodeIf(rxPort)
          ->getInterfaceID());

  std::vector<std::pair<folly::IPAddressV6, folly::MacAddress>> macIPv6Pairs;

  for (int i = 0; i < FLAGS_max_l2_entries; ++i) {
    std::stringstream ipStream;
    ipStream << "2620:0:1cfe:face:b10c::" << std::hex << i;
    folly::IPAddressV6 ip(ipStream.str());
    uint64_t macBytes = kBaseMac;
    folly::MacAddress mac = folly::MacAddress::fromHBO(macBytes + i);
    macIPv6Pairs.push_back(std::make_pair(ip, mac));
  }
  const int srcPort = 47231;
  const int dstPort = 277;
  for (const auto& [ipV6, mac] : macIPv6Pairs) {
    auto txPacket = utility::makeTCPTxPacket(
        ensemble->getSw(),
        vlan,
        mac,
        // MacAddress::BROADCAST send from second last interface to last
        // if replaced by macLastInterface, then traffic won't flood
        rxInterfaceMac,
        ipV6,
        folly::IPAddressV6("2620:0:1cfe:face:b10b::"),
        srcPort,
        dstPort);
    ensemble->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), txPort);
  }
}
void configureMaxMacEntries(AgentEnsemble* ensemble) {
  auto asic =
      utility::checkSameAndGetAsic(ensemble->getHwAsicTable()->getL3Asics());
  // TH3 had existing slowness of l2 callbacks. To exercise the callback path,
  // we utilize l2 callback on sw switch to simulate large scale of l2 callback
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK3) {
    configureMaxMacEntriesViaL2LearningUpdate(ensemble);
  } else {
    configureMaxMacEntriesViaPacketIn(ensemble);
  }
}

void syncFib(
    SwSwitchRouteUpdateWrapper& updater,
    const RouterID kRid,
    const utility::RouteDistributionGenerator::ThriftRouteChunk& routes) {
  std::for_each(
      routes.begin(), routes.end(), [&updater, kRid](const auto& route) {
        updater.addRoute(kRid, ClientID::BGPD, route);
      });
  updater.program(
      {{{kRid, ClientID::BGPD}},
       RouteUpdateWrapper::SyncFibInfo::SyncFibType::IP_ONLY});
}

void configureMaxRouteEntries(AgentEnsemble* ensemble) {
  auto switchIds = ensemble->getHwAsicTable()->getSwitchIDs();
  CHECK_EQ(switchIds.size(), 1);
  auto asic =
      utility::checkSameAndGetAsic(ensemble->getHwAsicTable()->getL3Asics());
  auto* sw = ensemble->getSw();
  // reserve 10 ecmp for scaling routes
  auto const kMaxEcmpGropus =
      utility::getMaxEcmpGroups(ensemble->getL3Asics()) *
          FLAGS_ecmp_resource_percentage / 100 -
      10;

  auto maxNumRoute =
      asic->getMaxRoutes().has_value() ? asic->getMaxRoutes().value() : 0;
  CHECK_GT(maxNumRoute, 0);

  auto routeGenerator = ScaleTestRouteScaleGenerator(sw->getState());
  auto allThriftRoutes = routeGenerator.allThriftRoutes();
  int numThriftRoutes = allThriftRoutes.size();

  std::vector<RoutePrefixV6> ecmpPrefixes;
  utility::EcmpSetupTargetedPorts6 ecmpHelper(ensemble->getProgrammedState());
  std::vector<PortID> portIds = ensemble->masterLogicalInterfacePortIds();
  std::vector<PortDescriptor> portDescriptorIds;

  for (const auto& portId : portIds) {
    portDescriptorIds.push_back(PortDescriptor(portId));
  }

  std::vector<std::vector<PortDescriptor>> allCombinations =
      utility::generateEcmpGroupScale(
          portDescriptorIds, kMaxEcmpGropus, portDescriptorIds.size());
  std::vector<flat_set<PortDescriptor>> nhopSets;
  for (const auto& combination : allCombinations) {
    nhopSets.emplace_back(combination.begin(), combination.end());
  }

  XLOG(DBG2) << "Max Ecmp Groups: " << kMaxEcmpGropus
             << " Default Ecmp Width: " << kDefaulEcmpWidth
             << " combination size: " << allCombinations.size()
             << " generated routes: " << allThriftRoutes.size()
             << " routes assigned to ecmp group: " << allCombinations.size();

  // ECMP routes + generated routes should be less than max routes
  CHECK_LE(allThriftRoutes.size() + allCombinations.size(), maxNumRoute);

  ensemble->applyNewState(
      [&portDescriptorIds,
       &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(
            in,
            flat_set<PortDescriptor>(
                std::make_move_iterator(portDescriptorIds.begin()),
                std::make_move_iterator(portDescriptorIds.end())));
      },
      "program ecmp",
      false);

  std::generate_n(
      std::back_inserter(ecmpPrefixes), kMaxEcmpGropus, [i = 0]() mutable {
        return RoutePrefixV6{
            folly::IPAddressV6(folly::to<std::string>(2401, "::", i++)), 128};
      });

  auto wrapper = sw->getRouteUpdater();
  ecmpHelper.programRoutes(&wrapper, nhopSets, ecmpPrefixes);

  auto platformType = sw->getPlatformType();
  if (!routeGenerator.isSupported(platformType)) {
    throw FbossError(
        "Route scale generator not supported for platform: ", platformType);
  }

  ThriftHandler handler(ensemble->getSw());
  // Sync fib with same set of routes
  // TH doesn't support 75K route scale, the following is a work around
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
    std::vector<UnicastRoute> quarterThriftRoutes = std::vector<UnicastRoute>(
        allThriftRoutes.begin(),
        allThriftRoutes.begin() + allThriftRoutes.size() / 4);
    std::unique_ptr<std::vector<UnicastRoute>> routesPtr =
        std::make_unique<std::vector<UnicastRoute>>(quarterThriftRoutes);
    StopWatch timer("program_routes_msecs", FLAGS_json);
    handler.syncFib(static_cast<int>(ClientID::BGPD), std::move(routesPtr));
  } else {
    std::unique_ptr<std::vector<UnicastRoute>> routesPtr =
        std::make_unique<std::vector<UnicastRoute>>(allThriftRoutes);
    StopWatch timer("program_routes_msecs", FLAGS_json);
    handler.syncFib(static_cast<int>(ClientID::BGPD), std::move(routesPtr));
  }
  int route_count = 0;
  auto countRoutes = [&route_count](RouterID, auto&) { ++route_count; };
  forAllRoutes(sw->getState(), countRoutes);
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
    CHECK_GE(route_count, numThriftRoutes / 4);
  } else {
    CHECK_GE(route_count, numThriftRoutes);
  }
}

void removeAllRouteEntries(AgentEnsemble* ensemble) {
  const RouterID kRid(0);
  auto updater = ensemble->getSw()->getRouteUpdater();
  syncFib(updater, kRid, {});
  int route_count = 0;
  auto countRoutes = [&route_count](RouterID, auto&) { ++route_count; };
  forAllRoutes(ensemble->getSw()->getState(), countRoutes);
  auto directlyConnectedInterfaces =
      ensemble->getProgrammedState()->getInterfaces()->numNodes();
  CHECK_LE(route_count, 2 * (directlyConnectedInterfaces + 1) + 1);
}

void configureMaxAclEntries(AgentEnsemble* ensemble) {
  auto cfg = ensemble->getCurrentConfig();
  const auto maxAclEntries = getMaxAclEntries(ensemble->getL3Asics());
  XLOG(DBG2) << "Max Acl Entries: " << maxAclEntries;
  std::vector<cfg::AclTableQualifier> qualifiers =
      setAclQualifiers(AclWidth::SINGLE_WIDE);

  utility::addAclTable(&cfg, "aclTable0", 0 /* priority */, {}, qualifiers);
  addAclEntries(maxAclEntries, AclWidth::SINGLE_WIDE, &cfg, "aclTable0");

  ensemble->applyNewConfig(cfg);
}

void addPort2NewVlan(cfg::SwitchConfig& config, PortID portID) {
  auto vlanIfExist = std::find_if(
      config.vlans()->begin(), config.vlans()->end(), [](auto vlan) {
        return vlan.id() == kBaseVlanId + 1;
      });
  if (vlanIfExist != config.vlans()->end()) {
    throw FbossError("The vlan to add already exists");
  }

  // add vlan
  auto newVlan = cfg::Vlan();
  newVlan.name() = "rx tx test";
  newVlan.id() = VlanID(kBaseVlanId + 1);
  newVlan.routable() = true;
  config.vlans()->push_back(newVlan);

  // change the vlan id of ingressVlan of port
  for (auto& port : *config.ports()) {
    if (port.logicalID() == static_cast<int>(portID)) {
      port.ingressVlan() = kBaseVlanId + 1;
    }
  }

  // change vlan port mapping
  auto vlanPort = std::find_if(
      config.vlanPorts()->begin(),
      config.vlanPorts()->end(),
      [portID](auto vlanPort) {
        return vlanPort.logicalPort() == static_cast<int>(portID);
      });
  if (vlanPort == config.vlanPorts()->end()) {
    throw FbossError("The port not not found in vlan port mapping");
  }
  vlanPort->vlanID() = kBaseVlanId + 1;

  auto interfaceIfExist = std::find_if(
      config.interfaces()->begin(),
      config.interfaces()->end(),
      [](auto interface) { return interface.intfID() == kBaseVlanId + 1; });
  if (interfaceIfExist != config.interfaces()->end()) {
    throw FbossError("The vlan interface to add already exists");
  }

  // add l3 interface to vlan
  auto newInterface = cfg::Interface();
  newInterface.intfID() = kBaseVlanId + 1;
  newInterface.vlanID() = kBaseVlanId + 1;
  newInterface.routerID() = 0;
  newInterface.mac() = utility::kLocalCpuMac().toString();
  newInterface.mtu() = 9000;
  newInterface.ipAddresses()->resize(2);
  newInterface.ipAddresses()[0] = "192.1.1.1/24";
  newInterface.ipAddresses()[1] = "2001::1/64";
  config.interfaces()->push_back(newInterface);
}

cfg::SwitchConfig getSystemScaleTestSwitchConfiguration(
    const AgentEnsemble& ensemble) {
  FLAGS_sai_user_defined_trap = true;
  auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
  auto asic = utility::checkSameAndGetAsic(l3Asics);
  auto config = utility::oneL3IntfNPortConfig(
      ensemble.getSw()->getPlatformMapping(),
      asic,
      ensemble.masterLogicalPortIds(),
      ensemble.getSw()->getPlatformSupportsAddRemovePort(),
      asic->desiredLoopbackModes());

  utility::addAclTableGroup(
      &config, cfg::AclStage::INGRESS, utility::kDefaultAclTableGroupName());
  utility::addDefaultAclTable(config);
  // We don't want to set queue rate that limits the number of rx pkts
  utility::addCpuQueueConfig(
      config,
      ensemble.getL3Asics(),
      ensemble.isSai(),
      /* setQueueRate */ false);
  utility::setDefaultCpuTrafficPolicyConfig(
      config, ensemble.getL3Asics(), ensemble.isSai());

  config.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
  addPort2NewVlan(
      config, ensemble.masterLogicalInterfacePortIds()[kRxMeasurePortIdx]);
  return config;
};

void writeAgentConfigMarkerForFsdb() {
  auto filePath =
      folly::to<std::string>(FLAGS_write_agent_config_marker_for_fsdb);

  if (createFile(filePath) < 0) {
    XLOG(DBG2) << "Failed to create file: " << filePath;
    return;
  }

  while (true) {
    if (!checkFileExists(filePath)) {
      XLOG(DBG2) << "FSDB done with benchmarking and has deleted marker file";
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

void initSystemScaleTest(AgentEnsemble* ensemble) {
  configureMaxAclEntries(ensemble);
  configureMaxRouteEntries(ensemble);
  configureMaxMacEntries(ensemble);
  configureMaxNeighborEntries(ensemble);
  if (FLAGS_write_agent_config_marker_for_fsdb != "") {
    writeAgentConfigMarkerForFsdb();
  }
}

void initSystemScaleChurnTest(AgentEnsemble* ensemble) {
  configureMaxAclEntries(ensemble);

  std::vector<PortID> portToFlap = {
      ensemble->masterLogicalInterfacePortIds()[0]};
  auto portFlapHelper = PortFlapHelper(ensemble, portToFlap);
  CHECK_GT(ensemble->masterLogicalInterfacePortIds().size(), 2);
  auto macLearningFloodHelper = MacLearningFloodHelper(
      ensemble,
      ensemble->masterLogicalInterfacePortIds()[kMacChurnTxPortIdx],
      ensemble->masterLogicalInterfacePortIds()[kMacChurnRxPortIdx],
      VlanID(kBaseVlanId));
  configureMaxMacEntries(ensemble);
  portFlapHelper.startPortFlap();
  macLearningFloodHelper.startChurnMacTable();

  for (auto i = 0; i < kNumChurn; i++) {
    configureMaxRouteEntries(ensemble);
    removeAllRouteEntries(ensemble);
    configureMaxNeighborEntries(ensemble);
    removeAllNeighbors(ensemble);
  }
  portFlapHelper.stopPortFlap();
  macLearningFloodHelper.stopChurnMacTable();
}

} // namespace facebook::fboss::utility
